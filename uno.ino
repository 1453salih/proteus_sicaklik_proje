#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

const int fanPin = 9;    // Fan, 9 numaralı pine bağlı
const int potPin = A0;   // Potansiyometre, A0 numaralı pine bağlı
const int buttonPin = 2; // Buton, 2 numaralı pine bağlı
const int speakerPin = 6; // Hoparlör, 6 numaralı pine bağlı

LiquidCrystal_I2C lcd(0x20, 40, 4);

byte dataBuffer[4] = {0};

union
{
  float rxTemp;
  byte rxArray[4];
} rxData;

int tSetValue;         // Kaydedilen Tset değeri
int temperature;       // Ölçülen sıcaklık değeri

bool isWarningOn = false; // Uyarı sesi durumu

void setup(void)
{
  Serial.begin(9600);
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV128);
  digitalWrite(SS, LOW);

  lcd.begin(40, 4);   // LCD'yi başlat
  lcd.backlight();

  // İstediğiniz bilgileri LCD'ye yazdırma
  lcd.setCursor(0, 0); 
  lcd.print("Hitit Universitesi");
  lcd.setCursor(0, 1);
  lcd.print("Isim: Salim Huca");
  lcd.setCursor(0, 2);
  lcd.print("Okul No: 204210774");
  
  // Ekranda bu bilgilerin bir süre kalmasını sağlayabilirsiniz
  delay(3000); // 5 saniye bekler
  // LCD ekranını temizle
  lcd.clear();

  lcd.begin(40, 4);   // LCD'yi başlat
  lcd.backlight();

  pinMode(buttonPin, INPUT);
  pinMode(speakerPin, OUTPUT);

  tSetValue = EEPROM.read(0);  // EEPROM'dan Tset değerini oku
  if (tSetValue < 0 || tSetValue > 100)
  {
    // Geçersiz Tset değeri, varsayılan olarak 0 ata
    tSetValue = 0;
  }

  // Tset değerini LCD'ye yazdır
  lcd.setCursor(0, 3);
  lcd.print("Tset: " + String(tSetValue) + " C");
}

void loop(void)
{
  SPI.transfer(dataBuffer, sizeof(dataBuffer));

  rxData.rxArray[0] = dataBuffer[1];
  rxData.rxArray[1] = dataBuffer[2];
  rxData.rxArray[2] = dataBuffer[3];
  rxData.rxArray[3] = dataBuffer[0];

  String tempString = String(rxData.rxTemp, 2); // Sıcaklığı stringe çevir

  lcd.setCursor(0, 0); // İmleci 0. sütun, 0. satıra yerleştir
  lcd.print("Temp: " + tempString + " C"); // Sıcaklığı yazdır

  lcd.setCursor(0, 1);
  if (rxData.rxTemp >= 0 && rxData.rxTemp <= 15)
  {
    lcd.print("Durum: Soguk");
    analogWrite(fanPin, 0); // Fanı kapat
    
    lcd.setCursor(0, 2);
    lcd.print("Fan: Kapali"); // Fanın kapalı olduğunu belirt
    
    stopWarningSound(); // Uyarı sesini durdur
    
  }
  else if (rxData.rxTemp >= 16 && rxData.rxTemp <= 30)
  {
    lcd.print("Durum: Normal");
    analogWrite(fanPin, 0); // Fanı kapat
    
    lcd.setCursor(0, 2);
    lcd.print("Fan: Kapali"); // Fanın kapalı olduğunu belirt
    
    stopWarningSound(); // Uyarı sesini durdur
  }
  else if (rxData.rxTemp >= 31 && rxData.rxTemp <= 100)
  {
    lcd.print("Durum: Sicak");
    int pwmVal = map(rxData.rxTemp, 31, 100, 0, 255); // Sıcaklığı PWM değerine dönüştür
    analogWrite(fanPin, pwmVal); // PWM ile fanı aç
    
    lcd.setCursor(0, 2);
    if(pwmVal == 0)
    {
      lcd.print("Fan: Kapali"); // Fanın kapalı olduğunu belirt
    }
    else
    {
      lcd.print("Fan: Acik, " + String(pwmVal * 100 / 255) + "%"); // Fan durumunu yazdır
    }
    // Buton durumunu kontrol et
    if (digitalRead(buttonPin) == HIGH)
    {
      // Butona basıldıysa, Tset değerini güncelle ve EEPROM'a kaydet
      tSetValue = EEPROM.read(0);  // EEPROM'dan Tset değerini oku
      int potValue = analogRead(potPin);
      int tset = map(potValue, 0, 1023, 0, 100);  // 0-1023 aralığındaki potansiyometre değerini 0-100 aralığına dönüştür
      if (tset != tSetValue)
      {
        tSetValue = tset;
        EEPROM.write(0, tSetValue);  // Tset'i EEPROM'a kaydet
        // Tset değerini LCD'ye yazdır
        lcd.setCursor(0, 3);
        lcd.print("Tset: " + String(tSetValue) + " C");
      }
      delay(200);  // Buton debouncing
    }

    // Sıcaklık Tset değerini aştığında uyarı sesi çal
    if (rxData.rxTemp > tSetValue)
    {
      startWarningSound();
    }
    else
    {
      stopWarningSound();
    }
  }
  else
  {
    analogWrite(fanPin, 0); // Sıcaklık beklenen aralığın dışındaysa fanı kapat
    lcd.setCursor(0, 2);
    lcd.print("Fan: Kapali"); // Fanın kapalı olduğunu belirt
    stopWarningSound(); // Uyarı sesini durdur
  }

  Serial.print("SPI Portundan alinan sicaklik: ");
  Serial.print(rxData.rxTemp, 2);
  Serial.println(" °C");
  Serial.println("============================================");
}

void startWarningSound()
{
  if (!isWarningOn)
  {
    int note = 1000; // Başlangıç frekansı
    int duration = 100; // Not süresi

    for (int i = 0; i < 10; i++)
    {
      tone(speakerPin, note, duration); // Kesik kesik ses çal

      delay(100); // Not arası bekleme süresi

      noTone(speakerPin); // Sesi durdur

      delay(100); // Not sonrası bekleme süresi

    }

    isWarningOn = true;
  }
}


void stopWarningSound()
{
  if (isWarningOn)
  {
    noTone(speakerPin); // Hoparlörden gelen sesi durdur

    isWarningOn = false;
  }
}

