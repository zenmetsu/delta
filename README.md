# delta
## Digital EtaLon Tuning Assistant

### Description
The goal of the project is to provide a digital tuning aid to users of Lunt pressure-tuned Hydrogen alpha solar telescopes.  While tuning a single etalon with a imaging device is somewhat trivial, judging the tuning visually is significantly more challenging.  Moreover, the addition of a second etalon complicates matters since it becomes almost impossible to discern the tuning effects of two etalons in series.

### Status
This project is currently in prototype status with an Adafruit Feather M0 Adalogger as the microcontroller, paired with a BME280 Pressure/Temperature/Humidity sensor and a 7-LED NeoPixel display.  

The NeoPixel was chosen due to the high brightness and low power consumption.  As solar observation necessarily takes place during the day, OLED and LCD displays are a poor choice.  Unfortunately, displaying information on a limited number of pixels is difficult.  There is currently a numbering system described in the code, the gist of which is that the LEDs in the outer ring count as "1" and the center LED counts as a "7".  Simply add to find the value, with "10" representing zero and "13", which corresponds to all LEDs lit, representing a decimal point.

### Theory of operation
A pressure tuned etalon will pass frequencies that have a vacuum wavelength that are a multiple of 2 * d * n, where 'd' represents the etalon spacing and 'n' represents the index of refraction for the gap.  By using a camera with a single pressure-tuned etalon to tune to a minimum brightness, the etalon will be "on-band" as evidenced by maximum attenuation of the photosphere.  At this tuning point, the index of refraction can be calculated using an enhanced Ciddor equation from the measured pressure, temperature, and relative humidity readings from the BME280 sensor.  From the known index of refraction 'n', 'd' can be calculated and the etalon gap will be known to be a multiple thereof.  As this gap spacing does not undergo significant changes, the vacuum wavelength that the etalon is tuned to becomes a function of 'n'.  The user can thus be given visual or audio cues to tune the etalon to the vacuum wavelength of Hydrogen alpha which is 656.46nm.



