/*
 * Challenge 01 — Use After Free (심화: vtable 기반 위젯 시스템)
 *
 * [시나리오]
 *   아주 작은 GUI 흉내. 각 위젯(Widget)은 힙 객체이며 첫 멤버로 "vtable"
 *   (render/on_event 함수 포인터 묶음)을 가진다. Screen 은 위젯 포인터 배열을
 *   들고 있고, 이벤트를 나눠준 뒤(dispatch) 한 프레임을 그린다(render).
 *
 * [기대 동작]
 *   버튼/라벨/다이얼로그를 그리고, 닫기 이벤트 후 남은 위젯(버튼, 라벨)만 다시 그린 뒤
 *   정상 종료(0).
 *
 * [증상]
 *   닫기 이벤트 핸들러가 다이얼로그 위젯을 free() 하지만, Screen 의 포인터 배열에서
 *   그 슬롯을 제거(NULL 로)하지 않는다. 그 사이 앱이 상태 메시지 버퍼를 새로 할당하며
 *   방금 해제된 청크를 재사용해 vtable 포인터 자리를 덮어쓴다.
 *   다음 렌더 패스에서 해제된 위젯의 w->vtbl->render 를 호출 → 망가진 함수 포인터로
 *   점프 → SIGSEGV. 크래시는 render 루프에서 나지만, 원인은 멀리 떨어진 close 핸들러다.
 *
 * [gdb 로 잡기]
 *   make gdb NAME=01_use_after_free
 *   (gdb) run                         → 크래시(SIGSEGV)
 *   (gdb) bt                          → screen_render() 안 w->vtbl->render(w) 지점
 *   (gdb) print w                     → 어떤 위젯인지(주소/슬롯) 확인
 *   (gdb) print w->vtbl               → 오염돼 있음
 *   (gdb) print s->items[2]           → 이미 해제된 슬롯이 그대로 남아있음
 *   (gdb) break widget_destroy        → 누가/언제 이 위젯을 free 하는지 역추적
 *
 * [printf(로그)로 잡기]
 *   위젯 해제 시점과 렌더 시점의 vtbl 값을 각각 찍어 "해제가 사용보다 먼저"인지 확인:
 *     (destroy) fprintf(stderr, "destroy id=%d w=%p vtbl=%p\n", w->id,(void*)w,(void*)w->vtbl);
 *     (render)  fprintf(stderr, "render  id=%d w=%p vtbl=%p\n", w->id,(void*)w,(void*)w->vtbl);
 *   → 같은 주소가 destroy 후 render 에서 다시 나오고, vtbl 값이 달라져 있으면 UAF.
 *   (stdout 은 버퍼링되니 stderr 로 찍어야 크래시 직전 로그가 남는다)
 *
 * TODO: "해제"와 "슬롯 정리"를 한 곳에서 같이 하세요. 위젯 자신은 Screen 을 모르므로
 *       (dialog_on_event 는 self 만 안다) 이벤트 핸들러에서는 closed 표시만 남기고,
 *       Screen 쪽에서 closed 위젯을 free 한 뒤 그 슬롯을 NULL 로 만드는 편이 자연스럽습니다. 
 *       이후 dispatch/render 루프가 NULL 슬롯을 건너뛰게 하세요. "해제 = 소유 포인터 무효화".
 *       => dialog_on_event에서 메모리 해제한 후 dispatch에서 슬롯 NULL처리하는 것과 무슨 차이?
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Widget Widget;

// Widget을 매개 변수로 받는 함수 포인터 모음
typedef struct {
    void (*render)(Widget *self);
    void (*on_event)(Widget *self, int code);
} VTable;

// 위젯은 V테이블과 id,closed,label을 변수로 갖는 구조체이다.
struct Widget {
    const VTable *vtbl; 
    int id;
    int closed;
    char label[24];
};

// 스크린은 최대 8개의 위젯을 갖는다.
// Q1. #define MAX_WIDGETS 8 이랑 const int MAX_COUNT=8과 차이는 뭐지? 
// Q2. 굳이 #define으로 정의한 이유는 뭘까? 
// A1. #define은 전처리기 매크로, const int는 변수
/*
 전자는 타입이 따로 없고, 메모리도 보통 없으며, 주소 접근(&)도 불가능하다. 
컴파일 전 치환되며 #if 에서 사용하고 배열 크기 등 컴파일 타임 상수로 사용 가능하다.
디버거에서 변수처럼 확인이 어렵다.
 반면 후자는 변수이고 int타입이며 메모리에 실제 객체가 존재한다. 주소 접근도 가능하다.
컴파일 전 치환이 불가하고, #if에서 사용하지 않는다. 경우에 따라 배열 크기 등 컴파일 타임 상수로 
사용하지 못할 수도 있다. 디버거에서 변수처럼 확인이 가능하다.
*/
// A2. 
/*
 가장 중요한 이유 중 하나는 컴파일 타임 상수로 확실하게 사용할 수 있기 때문이다.
const int MAX_COUNT = 8;가 항상 정수 상수 표현식으로 취급되지 않기 때문에
int arr[MAX_COUNT]; 가 C에서는 문제가 될 수 있다.

*/
#define MAX_WIDGETS 8
typedef struct {
    Widget *items[MAX_WIDGETS];
    int count;
} Screen;

/* ── 위젯 종류별 동작 ─────────────────────────────────────────── */
static void button_render(Widget *self) {
    printf("  [Button #%d] \"%s\"\n", self->id, self->label);
}
static void label_render(Widget *self) {
    printf("  Label #%d: %s\n", self->id, self->label);
}
static void dialog_render(Widget *self) {
    printf("  <<Dialog #%d>> %s\n", self->id, self->label);
}

static void widget_noop_event(Widget *self, int code) { (void)self; (void)code; }

/* 다이얼로그는 이벤트 코드 1(닫기)을 받으면 스스로 정리(파괴)된다 */
static void dialog_on_event(Widget *self, int code);

// 각 VTable 정의 
static const VTable BUTTON_VT = { button_render, widget_noop_event };
static const VTable LABEL_VT  = { label_render,  widget_noop_event };
static const VTable DIALOG_VT = { dialog_render, dialog_on_event  };

static Widget *widget_new(const VTable *vt, int id, const char *label) {

    /* [Thinking Point]
    *   w 에 아직 아무 값도 넣지 않았는데, sizeof *w 로 *w 를 써도 괜찮은 이유는?
    *   tip 1. sizeof 는 피연산자를 '실행(역참조)'하지 않고 '타입'만 본다.
    *          → *w 의 타입(Widget)만 필요할 뿐, w 를 실제로 따라가지 않는다.
    *   tip 2. 그래서 sizeof *w 는 (VLA 제외) 컴파일 타임에 sizeof(Widget) 상수로 치환된다.
    *   생각해보기: sizeof(Widget) 대신 sizeof *w 로 쓰면 어떤 장점이 있을까?
    */
    Widget *w = malloc(sizeof *w);

    // perror : C에서 시스템 함수가 실패했을 때, 현재 errno 값에 해당하는 오류 메시지를 출력하는 함수(print error)
    // perror("malloc") : 직전에 실패한 malloc()과 관련된 시스템 오류 메시지를 출력할 때 쓰는 형태
    // ex) malloc: Cannot allocate memory
    // exit(1) : 프로그램을 즉시 종료하면서 운영체제에 종료 코드 1을 반환하는 함수
    // 정상 종료는 보통 0을, 비정상 종료는 1을 사용
    // strncpy(dest, src, n); : dest에 src를 앞에서부터 n바이트 만큼 복사한다.
    // src길이가 n이상이면 종료 문자 '\0'가 자동으로 붙지 않기 때문에, 마지막 문자에 직접 추가해줘야 한다.
    if (!w) { perror("malloc"); exit(1); }
    w->vtbl = vt;
    w->id = id;
    w->closed = 0;
    strncpy(w->label, label, sizeof(w->label) - 1);
    w->label[sizeof(w->label) - 1] = '\0';
    return w;
}

static void widget_destroy(Widget *w) {
    free(w);          
}

/* ── Screen ──────────────────────────────────────────────────── */
static void screen_add(Screen *s, Widget *w) {
    if (s->count < MAX_WIDGETS) s->items[s->count++] = w;
}

// count <= MAX_WIDGETS 
// sol : 메모리 해제와 슬롯 NULL처리 한번에
// 위젯은 스크린을 모르기 때문에 파라미터가 스크린인 함수에서 처리
static void screen_dispatch(Screen *s, int code) {
    for (int i = 0; i < s->count; i++) {
        if (s->items[i] == NULL) // _ljw add
            continue;
        Widget *w = s->items[i];
        w->vtbl->on_event(w, code);
        if (w->closed == 1) {   // _ljw add
            widget_destroy(w); 
            s->items[i] = NULL;
        }
    }
}

// 입력된 스크린 주소에 위젯 출력
static void screen_render(Screen *s) {
    for (int i = 0; i < s->count; i++) {
        if (s->items[i] == NULL) // _ljw add
            continue;            
        Widget *w = s->items[i];
        w->vtbl->render(w);      
    }
}

// 코드를 1로 받으면 해당 위젯 메모리 해제
// 메모리 해제를 screen_dispatch로 위임
static void dialog_on_event(Widget *self, int code) {
    if (code == 1) {
        self->closed = 1;
        // widget_destroy(self); // _ljw comment out 
    }
}

static char *app_build_status(const char *text) {
    char *msg = malloc(sizeof(Widget));   
    if (!msg) exit(1);

    /* [테스트용 연출] 재사용한 메모리를 0xAB 로 '일부러' 덮어써서 오염시킨다.
     * 실무라면 다른 기능이 우연히 이 자리를 덮어쓰겠지만, 여기서는 UAF 크래시를
     * 매번 똑같이(결정적으로) 재현하기 위해 인위적으로 채운다. 
     * glibc(리눅스) 환경 (tcache)에서만 유효하다. 환경&상황에 따라 msg는 새로운 주소로 할당될 수 있다.
     */
    memset(msg, 0xAB, sizeof(Widget));
    snprintf(msg, sizeof(Widget), "STATUS: %s", text);
    return msg;
}

int main(void) {
    Screen s = { .count = 0 };

    // screen에 widget(위젯) 추가
    screen_add(&s, widget_new(&LABEL_VT,  10, "Welcome"));
    screen_add(&s, widget_new(&BUTTON_VT, 11, "OK"));
    screen_add(&s, widget_new(&DIALOG_VT, 12, "Are you sure?"));  /* items[2] */
    screen_add(&s, widget_new(&BUTTON_VT, 13, "Cancel"));

    printf("frame 1:\n");
    screen_render(&s);
    screen_dispatch(&s, 1);

    /* TODO 닫힌(closed) 위젯을 여기서 정리(free + 해당 슬롯 NULL)할 필요가 있음 */

    char *status = app_build_status("dialog closed");
    printf("%s\n", status);

    printf("frame 2:\n");
    screen_render(&s); // <- ※여기서 seg fault 걸림

    free(status);
    for (int i = 0; i < s.count; i++) free(s.items[i]);
    return 0;
}
