#define PWMCH_IMTX1	(1)
#define PWMCH_IMTX2	(0)
#define PWMCH_IMTX3	(4)
#define PWMCH_IMTX4	(6)
#define PWMCH_IBTX1	(3)
#define PWMCH_IBTX2	(2)
#define PWMCH_IBTX3	(7)
#define PWMCH_IBTX4	(5)

void PWM_Init(void); 
uint8_t PWM_Set(uint8_t channel,uint16_t pwmval);

uint8_t Imtx_Set(uint8_t imch,uint16_t pwmval);
uint8_t Ibtx_Set(uint8_t imch,uint16_t pwmval);
uint8_t Ibtx_Get(uint8_t ibch,uint16_t *pwmval);
uint8_t Ibmx_Get(uint8_t ibch,uint16_t *pwmval);
