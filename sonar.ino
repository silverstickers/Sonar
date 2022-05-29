#define MY_SERIAL SerialUSB

constexpr size_t g_pinToSamex[10] = {7, 6, 5, 4, 3, 2, 1, 0, 10, 11}; // Pin A0 -> A9 are named differently in the CPU
constexpr uint32_t g_enabledPins [] = {7, 6, 5, 4};
constexpr size_t g_numEnabledPins = sizeof(g_enabledPins)/sizeof(*g_enabledPins);
uint32_t g_enabledPinsBinary = 0;
constexpr size_t g_usPerSample = 1;
constexpr size_t g_samplesPerPin = 8000 / g_numEnabledPins / g_usPerSample; // total sample time, divided over g_numEnabledPins and with 1 µs sample time per sample


#define CYCLES_PER_LOOP 3
inline void wait_cycles( uint32_t n ) {
    uint32_t l = n/CYCLES_PER_LOOP;
    asm volatile( "0:" "SUBS %[count], 1;" "BNE 0b;" :[count]"+r"(l) );
}


void sendPing() {
    // noInterrupts();
    digitalWrite(53, HIGH);
    wait_cycles(850);
    digitalWrite(53, LOW);
    wait_cycles(850);

    digitalWrite(53, HIGH);
    wait_cycles(850);
    digitalWrite(53, LOW);
    wait_cycles(850);

    digitalWrite(53, HIGH);
    wait_cycles(850);
    // wait_cycles(300);
    digitalWrite(53, LOW);
    wait_cycles(850);
    // interrupts();
}


void readReceivers(uint16_t (&waveforms)[g_numEnabledPins][g_samplesPerPin]) {

    for(int sample = 0; sample < g_samplesPerPin; sample++){
        while((ADC->ADC_ISR & g_enabledPinsBinary) != g_enabledPinsBinary);// wait for conversions of enabled pins
        for (int pin = 0; pin < g_numEnabledPins; pin++) {
            waveforms[pin][sample] = ADC->ADC_CDR[g_pinToSamex[pin]];
        }
    }
}




void setup()
{
    pinMode(53, OUTPUT);
    digitalWrite(53, LOW);


    MY_SERIAL.begin(115200);
    while (!MY_SERIAL) {
    ; // wait for serial port to connect. Needed for native USB port only
    }
    MY_SERIAL.setTimeout(1);

    REG_ADC_MR = 0x10380180; // change from 10380200 to 10380180, 1 is the PRESCALER and 8 means FREERUN
    ADC->ADC_MR |= 0x80; // these lines set free running mode on enabled adc pins
    ADC->ADC_CR=2;
    for (auto p : g_enabledPins) {
        g_enabledPinsBinary |= (1 << p);
    }
    ADC->ADC_CHER = g_enabledPinsBinary;
}




void loop()
{
    sendPing();
    const auto start = micros();
    // delayMicroseconds(10);
    uint16_t waveforms [g_numEnabledPins][g_samplesPerPin] = {0};
    readReceivers(waveforms);
    auto elapsed = micros() - start;

    for (int i = 0; i < 25; i++) {
        MY_SERIAL.write((uint8_t)1);
    }
    for (int i = 0; i < 25; i++) {
        MY_SERIAL.write((uint8_t)200);
    }
    for (int i = 0; i < 25; i++) {
        MY_SERIAL.write((uint8_t)31);
    }
    for (int i = 0; i < 25; i++) {
        MY_SERIAL.write((uint8_t)41);
    }
    MY_SERIAL.print("Time to read ");
    MY_SERIAL.print(g_numEnabledPins*g_samplesPerPin);
    MY_SERIAL.print(" samples (us): ");
    MY_SERIAL.println(elapsed);
    
    const auto num_bytes = g_samplesPerPin * 2;
    MY_SERIAL.write(reinterpret_cast<const byte*>(&num_bytes), 4);
    MY_SERIAL.write(reinterpret_cast<const byte*>(&g_numEnabledPins), 4);
    for (auto wave : waveforms) {
        MY_SERIAL.write(reinterpret_cast<byte*>(wave), num_bytes);
    }

    elapsed = micros() - start;
    MY_SERIAL.print("Time for complete read / send in us: ");
    MY_SERIAL.println(elapsed);

    delay(500 > elapsed / 1000 ? 500 - elapsed / 1000 : 1);
}

