/*
 * Challenge 05 — NULL 반환 미확인 역참조 (심화: 설정 템플릿 확장기)
 *
 * [시나리오]
 *   key=value 설정 저장소(Config)와, "${key}" 자리표시자를 실제 값으로 치환하는
 *   템플릿 확장기 expand() 를 만든다. 예: "http://${host}:${port}/${path}".
 *
 * [기대 동작]
 *   템플릿의 모든 ${key} 를 설정값으로 치환한 최종 문자열을 출력.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_KV 16

// 20바이트 : 8 + 8 + 4
typedef struct {
    const char *keys[MAX_KV];
    const char *vals[MAX_KV];
    int n;
} Config;

// Config key , value, n 셋팅
static void cfg_set(Config *c, const char *k, const char *v) {
    if (c->n < MAX_KV) { c->keys[c->n] = k; c->vals[c->n] = v; c->n++; }
}

// keys , vals 순회하여 반환
static const char *cfg_get(const Config *c, const char *k) {
    for (int i = 0; i < c->n; i++)
        if (strcmp(c->keys[i], k) == 0) return c->vals[i];
    return NULL;                       /* 없는 키 → NULL */
}

// 
static void expand(const Config *c, const char *tmpl, char *out, size_t outcap) {
    size_t o = 0;
    for (const char *p = tmpl; *p; ) {
        if (p[0] == '$' && p[1] == '{') {
            const char *end = strchr(p, '}');
            if (!end) break;
            char key[32];
            size_t kl = (size_t)(end - (p + 2));
            if (kl >= sizeof key) kl = sizeof key - 1;
            memcpy(key, p + 2, kl);
            key[kl] = '\0';

            const char *v = cfg_get(c, key);      
            size_t vl = strlen(v);                 
            if (o + vl < outcap) { memcpy(out + o, v, vl); o += vl; }
            p = end + 1;
        } else {
            if (o + 1 < outcap) out[o++] = *p;
            p++;
        }
    }
    out[o] = '\0';
}

int main(void) {
    /* [Thinking Point]
     * "{ .n = 0 }" 은 멤버 이름을 콕 집어 초기화하는 '지정 초기화자(designated initializer)'다.
     *   tip 1. 초기화자에 하나라도 값을 주면, 명시하지 않은 나머지 멤버는 전부 0 으로
     *          채워진다. 즉 keys[], vals[] 배열도 모두 NULL 로 초기화된다.
     *   tip 2. 만약 그냥 "Config cfg;" 로만 뒀다면 지역 변수라 n·keys·vals 가 쓰레기 값이다.
     *   생각해보기: n 이 쓰레기 값이면 cfg_set/cfg_get 에서 무슨 일이 벌어질까?
     * 
     *               */
    Config cfg = { .n = 0 };
    cfg_set(&cfg, "host", "example.com");
    cfg_set(&cfg, "port", "8080");

    /* [Thinking Point]
     * "${host}" 처럼 ${...} 로 감싼 부분은 expand 함수가 설정값으로 치환하는 'placeholder' 다.
     *   tip 1. 이 문자열 자체는 그냥 상수 텍스트일 뿐, 컴파일러가 ${...} 를 해석하지 않는다.
     *          실제 치환은 런타임에 expand 함수 안에서 키를 찾아 값을 끼워넣는 방식으로 일어난다.
     *   tip 2. cfg_get("path") 는 등록되지 않은 키라 NULL 을 돌려준다.
     *   생각해보기: 설정에 없는 키(${path})를 만나면 expand() 는 어떤 값을 받게 되고,
     *               그 값을 검사 없이 strlen/복사에 쓰면 무슨 일이 벌어질까?
     *               (힌트: "값이 없다"는 NULL 이지 빈 문자열 ""이 아니다) */
    const char *tmpl = "http://${host}:${port}/${path}/index.html";
    char out[256];

    expand(&cfg, tmpl, out, sizeof out);   /* ${path} 치환 시 NULL 역참조 → 크래시 */

    printf("url = %s\n", out);
    return 0;
}
