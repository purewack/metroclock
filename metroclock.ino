#include <libmaple/libmaple.h>
#include <libmaple/usart.h>
#include <libmaple/dma.h>
#include <USBComposite.h>
USBMIDI midi;

int bpm = 0;
int beats = 4;
int beat = 0;
int ppq = 0;
bool snd = false;
bool playing = false;
bool bstate = false;

//2048 accuracy
const int srate = 23438;//hz
int16_t tick[1024];
int16_t tickLow[1024];
void setClick(int16_t* buf, double f){
  for(int i=0; i<1024; i++){
    double sc = min(-log(double(i)/1024.0),1.0);
    buf[i] = 1024+int16_t(0.3*sc*2048.0*sin(2.0*3.1415*f*double(i)/double(srate)));  
  }  
}


struct pwm_dac_t{
  dma_dev* dma;
  dma_channel dma_ch_a;
  dma_channel dma_ch_b;

  timer_dev* timer;

  uint8_t buf_len;
  uint8_t req = 0;
} pwm_dac;

void setDac(){
  pwm_dac.timer = TIMER2;
  pwm_dac.dma = DMA1;
  pwm_dac.dma_ch_a = DMA_CH5;
  pwm_dac.dma_ch_b = DMA_CH7;

  pinMode(PA0, PWM);
  pinMode(PA1, PWM);
  timer_pause(pwm_dac.timer);
  timer_set_prescaler(pwm_dac.timer, 0);
  timer_set_reload(pwm_dac.timer, 2048-1);
  timer_dma_enable_req(pwm_dac.timer, 1);
  //timer_dma_enable_req(pwm_dac.timer, 2);
  timer_resume(pwm_dac.timer);

  dma_init(pwm_dac.dma);
  dma_disable(pwm_dac.dma, pwm_dac.dma_ch_a);
}

void scheduleClick(void* buf, int len){
  __IO uint32 *tccr_a = &(pwm_dac.timer->regs).gen->CCR1;
  __IO uint32 *tccr_b = &(pwm_dac.timer->regs).gen->CCR2;
  dma_disable(pwm_dac.dma, pwm_dac.dma_ch_a);
  int m = DMA_FROM_MEM | DMA_MINC_MODE;
  dma_setup_transfer(pwm_dac.dma, pwm_dac.dma_ch_a , tccr_a, DMA_SIZE_16BITS, buf, DMA_SIZE_16BITS, m);
  dma_set_num_transfers(pwm_dac.dma, pwm_dac.dma_ch_a, len);  
  dma_enable(pwm_dac.dma, pwm_dac.dma_ch_a);
  //dma_enable(pwm_dac.dma, pwm_dac.dma_ch_b);
}

void midiSyncIRQ(){
  snd |= 1;
}

void tempoChange(unsigned int tempo, unsigned int bt){
  beats = bt;
  tempo *= 24;
  auto bpmrate = tempo / 60;

  uint32_t psc = 1;
  uint32_t arr = (48000000/bpmrate);
 
  while(arr > (1<<16)) {
    arr >>= 1;
    psc <<= 1;
  }

  timer_pause(TIMER3);
  timer_set_prescaler(TIMER3, psc-1);
  timer_set_reload(TIMER3, arr-1);
  timer_set_compare(TIMER3, TIMER_CH1, 1);
  timer_attach_interrupt(TIMER3, TIMER_UPDATE_INTERRUPT, midiSyncIRQ);
  timer_enable_irq(TIMER3, TIMER_UPDATE_INTERRUPT);
  timer_resume(TIMER3);    
}

void setup() {
  setClick(tick,2000.0);
  setClick(tickLow,1500.0);
  setDac();
  usart_init(USART1);
  usart_set_baud_rate(USART1,48000000,31250);
  usart_enable(USART1);
  gpio_set_mode(GPIOA,9,GPIO_AF_OUTPUT_PP);
  
  USBComposite.setProductId(0x0031);
  midi.begin();
  //while(!USBComposite);
  tempoChange(130,4);
  gpio_set_mode(GPIOB,10,GPIO_OUTPUT_PP);
  gpio_set_mode(GPIOB,11,GPIO_OUTPUT_PP);
  
//  gpio_set_mode(GPIOA,0,GPIO_INPUT_PU);
//  gpio_set_mode(GPIOA,1,GPIO_INPUT_PU);
//  gpio_set_mode(GPIOA,2,GPIO_INPUT_PU);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(snd){
    ppq = (ppq+1)%24;
    gpio_toggle_bit(GPIOB,10);
    if(!ppq) {
      beat = (beat+1)%beats;
      if(!beat)
        scheduleClick((void*)tick,1024);
      else
        scheduleClick((void*)tickLow,1024);
      gpio_toggle_bit(GPIOB,11);
    }
    midi.sendSync();
    uint8_t bt = 0xF8;
    usart_tx(USART1,&bt,1);
    snd = 0;
  }
//if(!gpio_read_bit(GPIOA,2) && !bstate){
//      bstate = true;
//      playing ? midi.sendStop() : midi.sendStart();
//      playing = !playing;
//      //delay(1000);
//    }
//
//    if(!gpio_read_bit(GPIOA,1) && !bstate){      
//      bstate = true;
//      bpm += 5;
//      tempoChange(bpm,4);
//      //delay(1000);
//    }
//
//    if(!gpio_read_bit(GPIOA,0) && !bstate){
//      bstate = true;
//      bpm -= 5;
//      tempoChange(bpm,4);
//      //delay(1000);
//    }
//  if(bstate){
//    bstate = !gpio_read_bit(GPIOA,0) | !gpio_read_bit(GPIOA,1) | !gpio_read_bit(GPIOA,2);
//  }
}
