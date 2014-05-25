/*
 * crate monitor
 */

#include <elapsedMillis.h>
#include <DHT.h>
#include <IODelegate.h>
#include <RunningMedian.h>
#include <SketchUtils.h>

// temp sensor
#define DHT_PIN 4
#define DHT_TYPE DHT22

#define DFLT_LOG_LEVEL LOG_LEVEL_DEBUG

// hall effect sensor
#define HALL_PIN 6

// light sensor
#define LIGHT_PIN A2

//motion sensor - red led
#define PIR_PIN 2
#define PIR_LED 11

// proximity sensor - green led
#define PROX_PIN A0
#define PROX_LED 12
#define PROX_LOW 100

boolean doorOpen = false;
boolean inCrate = false;

elapsedMillis timeElapsed;

DHT dht(DHT_PIN, DHT_TYPE);

int pirState = LOW;

// 5 seconds
unsigned long interval = 5000;

RunningMedian proxLast = RunningMedian(10);

void dhtSensor()
{
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(t) || isnan(h))
    {
        ioDelegate.error("failed to read values from dht sensor");
    }
    else
    {
        ioDelegate.debug("humdity [%f] - temperature [%f *C / %f *F]", h, t, (t * 1.8 + 32));
    }
}

void hallSensor()
{
    // high open, low closed
    if (digitalRead(HALL_PIN) == HIGH)
    {
        if (!doorOpen)
        {
            ioDelegate.debug("door opened");
            doorOpen = true;
        }
    }
    else
    {
        if (doorOpen)
        {
            ioDelegate.debug("door closed");
            doorOpen = false;
        }    
    }
}

void lightSensor()
{
    ioDelegate.debug("light sensor [%d]", analogRead(LIGHT_PIN));
}

void pirSensor()
{
    if (inCrate && digitalRead(PIR_PIN) == HIGH)
    {
        if (pirState == LOW)
        {
            pirState = HIGH;
            digitalWrite(PIR_LED, HIGH);

            ioDelegate.debug("motion detected");
        }
    }
    else
    {
        if (pirState == HIGH)
        {
            pirState = LOW;
            digitalWrite(PIR_LED, LOW);

            ioDelegate.debug("motion stopped");
        }
    }
}

void proxSensor()
{
    proxLast.add(analogRead(PROX_PIN));
    int val = proxLast.getMedian();

    if (val < 100)
    {
        if (inCrate)
        {
            inCrate = false;
            digitalWrite(PROX_LED, LOW);

            ioDelegate.debug("crate exitted");
        }
    }
    else
    {
        if (!inCrate)
        {
            inCrate = true;
            digitalWrite(PROX_LED, HIGH);

            ioDelegate.debug("create entered");
        }
    }
}

void loop()
{
    hallSensor();
    proxSensor();
    pirSensor();

    if (timeElapsed > interval)
    {
        dhtSensor();
        lightSensor();

        timeElapsed = 0;
    }
}

void setup()
{
    int inputPins[] = { PIR_PIN, DHT_PIN, HALL_PIN };
    sketchUtils.setAsInput(3, inputPins);

    int outputPins[] = { PIR_LED, PROX_LED };
    sketchUtils.setAsOutput(2, outputPins);

    dht.begin();
    
    // set the initial state of the create door
    doorOpen = digitalRead(HALL_PIN);
    
    ioDelegate.init(9600, DFLT_LOG_LEVEL);
    ioDelegate.debug(F("crate monitor initialized"));
}

