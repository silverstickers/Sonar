#define MY_SERIAL SerialUSB

constexpr size_t g_pinToSamex[10] = {7, 6, 5, 4, 3, 2, 1, 0, 10, 11}; // Pin A0 -> A9 are named differently in the CPU
constexpr uint32_t g_enabledPins [] = {7, 6};
constexpr size_t g_numEnabledPins = sizeof(g_enabledPins)/sizeof(*g_enabledPins);
uint32_t g_enabledPinsBinary = 0;

constexpr size_t g_samplesPerPin = 16000 / g_numEnabledPins / 1; // 16 ms total sample time, divided over g_numEnabledPins and with 1 µs sample time per sample

void setup() {
    analogWrite(DAC1, 200);

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

void loop() {
    uint16_t waveforms[g_numEnabledPins][g_samplesPerPin] = {0};

    const auto start = micros();
    for(int sample = 0; sample < g_samplesPerPin; sample++){
        while((ADC->ADC_ISR & g_enabledPinsBinary) != g_enabledPinsBinary);// wait for conversions of enabled pins
        for (int pin = 0; pin < g_numEnabledPins; pin++) {
            waveforms[pin][sample] = ADC->ADC_CDR[g_pinToSamex[pin]];
        }
    }
    const auto elapsed = micros() - start;

    MY_SERIAL.flush();
    for (int i = 0; i < 50; i++) {
        MY_SERIAL.write((uint8_t)1);
    }
    MY_SERIAL.print("Time to read ");
    MY_SERIAL.print(g_numEnabledPins*g_samplesPerPin);
    MY_SERIAL.print(" samples (us): ");
    MY_SERIAL.println(elapsed);
    MY_SERIAL.print("The first value is: "); MY_SERIAL.println(waveforms[0][0]);
    MY_SERIAL.print("The second value is: "); MY_SERIAL.println(waveforms[0][1]);
    
    const auto num_bytes = g_samplesPerPin * 2;
    MY_SERIAL.write(reinterpret_cast<const byte*>(&num_bytes), 4);
    MY_SERIAL.println(String(num_bytes));
    MY_SERIAL.write(reinterpret_cast<byte*>(waveforms[0]), num_bytes);
    MY_SERIAL.write(reinterpret_cast<byte*>(waveforms[1]), num_bytes);
    delay(3000);
    MY_SERIAL.print("Time for complete loop() in us: ");
    MY_SERIAL.println(micros()-start);
}