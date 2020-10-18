import unittest, logging
import azure.functions as func
from WeatherMeasTrg import main

class TestFunction(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        logging.basicConfig(level=logging.INFO)

    def test_main(self):
        evt = func.EventHubEvent(body=bytes('{"messageId":16, "Temperature":24.437500}', 'utf-8'))
        ret = main(evt)
        logging.info(ret)

        
        self.assertEqual(ret['Temperature'], 24.437500)