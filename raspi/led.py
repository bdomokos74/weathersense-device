import RPi.GPIO as GPIO
from time import sleep

LEDPIN=24

class Led:
    def __init__(self, ledPin=LEDPIN):
        self.ledPin = ledPin
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(ledPin, GPIO.OUT)

    def close(self):
        GPIO.cleanup([self.ledPin])

    def blinkLed(self, times=2):
        for _ in range(times):
            GPIO.output(self.ledPin, GPIO.HIGH)
            sleep(0.05)
            GPIO.output(self.ledPin, GPIO.LOW)
            if times>1: sleep(0.1)
