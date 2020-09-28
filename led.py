import RPi.GPIO as GPIO

LEDPIN=24

class Led:
    def __init__(ledPin=LEDPIN):
        self.ledPin = ledPin
        self.GPIO.setup(ledPin, GPIO.OUT)

    def clean():
        GPIO.cleanup([self.ledPin])

    def blinkLed(times=2):
        for _ in range(times):
            GPIO.output(ledPin, GPIO.HIGH)
            sleep(0.5)
            GPIO.output(ledPin, GPIO.LOW)
            if times>1: sleep(0.3)