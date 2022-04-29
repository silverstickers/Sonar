float waveform [2] = {LOW, HIGH};
int cursor = 0;

#define CYCLES_PER_LOOP 3
inline void wait_cycles( uint32_t n ) {
    uint32_t l = n/CYCLES_PER_LOOP;
    asm volatile( "0:" "SUBS %[count], 1;" "BNE 0b;" :[count]"+r"(l) );
}

void setup()
{
    pinMode(53, OUTPUT);
    digitalWrite(53, LOW);
}

void loop()
{
    noInterrupts();
    for (int i = 0; i < 6; i++) {
        digitalWrite(53, HIGH);
        wait_cycles(850);
        digitalWrite(53, LOW);
        wait_cycles(850);
    }
    interrupts();
    delay(3);
}
