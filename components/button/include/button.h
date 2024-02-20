#ifndef H_USER_BUTTONS_
#define H_USER_BUTTONS_
#ifdef __cplusplus
extern "C"
{
#endif


#define PIN_BIT(x) (1ULL<<x)

#define BUTTON_DOWN (1)
#define BUTTON_UP (2)

typedef struct {
	uint8_t pin;
    uint8_t event;
    uint64_t duration;
} button_event_t;

QueueHandle_t button_init(unsigned long long pin_select);

#ifdef __cplusplus
}
#endif

#endif // H_USER_BUTTONS_