/*
 * Copyright 2016 Herman verschooten
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * GPIO port implementation.
 *
 * This code is based upon the works of WiringPi.
 * see http://wiringpi.com
 * And elixir_ale by Frank Hunleth
 * see https://github.com/fhunleth/elixir_ale
 */

#include "erlcmd.h"
#include "gpio_port.h"
#include "gpio_port_rpi.h"
#include <unistd.h>
#include <stdlib.h>
#include <err.h>

#define DEBUG
#ifdef DEBUG
#define debug(...) do { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\r\n"); } while(0)
#else
#define debug(...)
#endif

#define HIGH 1
#define LOW 0
#define MAXTIMINGS  85

extern int gpio_read(    struct gpio *pin);
extern int gpio_write(   struct gpio *pin, unsigned int val);
extern int gpio_init(    struct gpio *pin, unsigned int pin_number, enum gpio_state dir);
extern int gpio_set_int( struct gpio *pin, const char *mode);
extern void gpio_pullup( struct gpio *pin, char *mode_str);

char *UP = "up";
char *DOWN = "down";
int dht11_dat[5] = { 0, 0, 0, 0, 0 };

int dht11_sense(struct gpio *pin) {
  dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;



  //expects pin to be in :input mode
  if (pin->state == GPIO_OUTPUT) 
    errx(EXIT_FAILURE, "Error. GPIO must be initialized as INPUT");

  if (gpio_read(pin) == HIGH) 
    errx(EXIT_FAILURE, "Error, GPIO must have the pullup set to UP before dht11_sense");

  debug("Send start signal");   
  debug ("Start state: %d", gpio_read( pin ));
  gpio_pullup(pin, DOWN);
  debug ("pulled DOWN: %d", gpio_read( pin ));
  usleep( 18 * 1000 );

  return 0;
}

//struct dht11Result dht11_sense(struct gpio *pin)
int dht11_sense_old(struct gpio *pin)
{
  //expects pin to be in :output mode
  if (gpio_read(pin) == 0) {
    gpio_write(pin, HIGH);
    sleep(1);
  }
  uint8_t currentstate, laststate = HIGH;
  uint8_t counter   = 0;
  uint8_t j   = 0, i;
  float f; /* fahrenheit */
  enum gpio_state dir = GPIO_OUTPUT;
  //struct dht11Result result;
  char meas [MAXTIMINGS][20];

  //int err;  // TODO: hanlde func errors  
  dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;
 
  /* pull pin down for 18 milliseconds */
  debug("sense start");
  debug ("Start state: %d", gpio_read( pin ));
  gpio_write(pin, LOW);
  debug ("pulled DOWN: %d", gpio_read( pin ));
  usleep( 18 * 1000 );

  /* then pull it up for 20-40 microseconds */
  gpio_write(pin, HIGH);
  debug ("Pull up and wait: %d", gpio_read( pin ));
  dir = GPIO_INPUT;
  if (gpio_init(pin, pin->pin_number, dir) < 0)
        errx(EXIT_FAILURE, "Error initializing GPIO as INPUT");
  gpio_set_int(pin, "falling");
  usleep( 20 ); // not sure that we need to wait, might miss first pullup from dht11

  /* detect change and read data */
  for ( i = 0; i < MAXTIMINGS; i++ )
  {
    counter = 0;
    //while ( digitalRead( DHTPIN ) == laststate )
    laststate = currentstate = gpio_read( pin );
    while ( currentstate == laststate)
    {
      counter++;
      usleep( 1 );
      if ( counter == 255 )
      {
        break;
      }
      currentstate = gpio_read( pin );
    }
    //laststate = digitalRead( DHTPIN );
    laststate = currentstate;

    sprintf(meas[i], "Laststate: %d (%u)", laststate, counter);

    if ( counter == 255 )
      break;
 
    /* ignore first 3 transitions */
    if ( (i >= 4) && (i % 2 == 0) )
    {
      /* shove each bit into the storage bytes */
      dht11_dat[j / 8] <<= 1;
      if ( counter > 16 )
        dht11_dat[j / 8] |= 1;
      j++;
    }

  }

  for (i = 0; i < MAXTIMINGS; i++)
  {
    debug("%s", meas[i]);
  }
  debug("sense polling finished");

  // Reset dht11 pin to high, to wait for next start signal.
  dir = GPIO_OUTPUT;
  if (gpio_init(pin, pin->pin_number, dir) < 0)
        errx(EXIT_FAILURE, "Error initializing GPIO as OUTPUT");
  gpio_write( pin, HIGH);


  /*
   * check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
   * print it out if data is good
   */
  if ( (j >= 40) &&
       (dht11_dat[4] == ( (dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF) ) )
  {
    f = dht11_dat[2] * 9. / 5. + 32;
    debug( "Humidity = %d.%d %% Temperature = %d.%d *C (%.1f *F)\n",
      dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3], f );
    return 10;
  }else {
    debug( "Data not good, skip\n" );
    debug( "Humidity = %d.%d %% Temperature = %d.%d *C\n",
      dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3]);
    return -1;
  }
}