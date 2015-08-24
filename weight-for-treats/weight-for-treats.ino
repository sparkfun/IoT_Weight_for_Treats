/**
 * Weight for Treats
 * Author: Shawn Hymel
 * Date: August 18, 2015
 * 
 * Run "HX711 Calibration" first to get the calibration factor number. Copy
 * that number to CALIBRATION_FACTOR below.
 * 
 * Wait for a dog to step onto the pressure plate (made with a load cell) and
 * dispense a treat. Wait for 2 seconds, take the weight of the dog (who is
 * hopefully completely on the scale), and post it to data.sparkfun.com.
 * 
 * Stream: https://data.sparkfun.com/sfe_dogs
 * 
 * License: http://opensource.org/licenses/MIT
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 */

#include "SparkFun-Spark-Phant/SparkFun-Spark-Phant.h"
#include "SparkFunMicroOLED/SparkFunMicroOLED.h"
#include "HX711.h""

// Parameters
#define DEBUG_PROGRAM       0       // 1 to output debug info to serial
#define CALIBRATION_FACTOR  -10700  // Run "HX711 Calibration" to get ths number
#define WEIGHT_THRESHOLD    1       // Number of pounds needed to dispense treats
#define POST_HOLDOFF        2000    // Time (ms) to wait before posting to data.
#define SERVO_START         90      // Starting position of servo
#define SERVO_END           1       // Ending position of servo
#define SERVO_DELAY         5       // Amount to delay between servo writes (ms)

// Pins
#define CLK_PIN             A0
#define DAT_PIN             A1
#define DISPENSER_PIN       D0

// data.sparkfun Parameters
const char server[] = "data.sparkfun.com";          // Phant destination server
const char publicKey[] = "n1369Z7jQZUzV0XMVo5d";    // Phant public key
const char privateKey[] = "Mox4k7XZ67ieGAmqGDdw";   // Phant private key
const char field[] = "weight";                      // Field for stream

// Global variables
Phant phant(server, publicKey, privateKey);
MicroOLED oled;
HX711 scale(DAT_PIN, CLK_PIN);
Servo dispenser;
float weight;
unsigned long dispense_time;
bool prev_treat;
bool post;

void setup() {
    
    // Initialize servo
    dispenser.attach(DISPENSER_PIN);
    dispenser.write(SERVO_START);

    // Set up serial to debug
#if DEBUG_PROGRAM
    Serial.begin(9600);
    while( !Serial.available() ) {
        Spark.process();
    }
    Serial.println("Weight for Treats");
#endif

    // Set scale factor and tare weight on scale
    scale.set_scale(CALIBRATION_FACTOR);
    scale.tare();
    
    // Initialize OLED, show splashscreen, and clear it
    oled.begin();
    oled.clear(ALL);
    oled.display();
    delay(500);
    oled.clear(PAGE);
    
    // Set initial flags
    prev_treat = false;
    post = false;
}

void loop() {
    
    // Take weight reading
    weight = scale.get_units();
    weight = -weight;
    if ( weight < 0 ) {
        weight = 0;
    }
    
    // Debug: print to serial
#if DEBUG_PROGRAM
    Serial.print("Reading: ");
    Serial.print(weight, 1);
    Serial.println(" lbs");
#endif

    // Show weight on OLED
    displayWeight(weight);
    
    // If we are over weight threshold, dispense treat
    if ( weight >= WEIGHT_THRESHOLD ) {
        
        // Only dispense if weight has been removed
        if ( prev_treat == false ) {
            dispenseTreat();
            dispense_time = millis();
            prev_treat = true;
            post = true;
        }
        
    } else {
        prev_treat = false;
    }
    
    // If we have reached our holdoff time, post weight
    if ( post && (millis() >= dispense_time + POST_HOLDOFF) ) {
        if ( weight >= WEIGHT_THRESHOLD ) {
#if DEBUG_PROGRAM
            Serial.println("Posting weight...");
#endif
            postToPhant(weight);
        }
        post = false;
    }
    
    // Wait for next cycle
    delay(50);
}

// Display weight (in lbs) on OLED
void displayWeight(float weight)
{
    
    // Clear and reset OLED
    oled.clear(PAGE);
    oled.setFontType(3);
    oled.setCursor(0, 0);
    
    // If we are over max display, just show 999.9
    if ( weight > 999.9 ) {
        oled.print("999.9");
        oled.display();
        return;
    }
    
    // Set unused digits to '0'
    if ( weight < 100 ) {
        oled.print(" ");
    }
    if ( weight < 10 ) {
        oled.print(" ");
    }
    
    // Print integer weight
    oled.print((int)weight);
    
    // Print (non-existent) decimal
    oled.print(" ");
    oled.circleFill(44, 45, 2);
    
    // Print 1/10s weight (rounded)
    weight = 10 * (weight - (int)weight);
    oled.print((int)(weight + 0.5));
    
    oled.display();
}

// Rotate servo back and forth to dispense a treat
void dispenseTreat() {
    
    rotateServo(SERVO_START, SERVO_END);
    delay(100);
    rotateServo(SERVO_END, SERVO_START);
}

// Rotate a servo from beginning to end position
void rotateServo(int begin, int end) {
    
    int pos;
    
    if ( begin > end ) {
        for ( pos = begin; pos >= end; pos--) {
            dispenser.write(pos);
            delay(SERVO_DELAY);
        }
    } else {
        for ( pos = begin; pos <= end; pos++) {
            dispenser.write(pos);
            delay(SERVO_DELAY);
        }
    }
}

// Post some piece of data to data.sparkfun
int postToPhant(float weight)
{
    
    // Use the OLED to show status
    oled.clear(PAGE);
    oled.setFontType(1);
    oled.setCursor(0, 0);
    
	// Convert weight to a string
	char weight_str[10];
	int weight_dec = (int)(weight);
	int weight_fr = (int)(((weight - weight_dec) * 10) + 0.5);
	if ( weight_fr == 10 ) {
	    weight_dec++;
	    weight_fr = 0;
	}
    sprintf(weight_str, "%d.%d", weight_dec, weight_fr);
#if DEBUG_PROGRAM
    Serial.print("Weight: ");
    Serial.println(weight_str);
#endif
 
	// Add weight to appropriate field before posting to Phant
    phant.add(field, weight_str);
	
	// Make a connection to data.sparkfun and post data
    TCPClient client;
    char response[512];
    int i = 0;
    int retVal = 0;
    
    if (client.connect(server, 80)) // Connect to the server
    {
		// Post message to indicate connect success
        oled.print("Posting");
        oled.display();
        oled.setCursor(0,17);
		
		// phant.post() will return a string formatted as an HTTP POST.
		// It'll include all of the field/data values we added before.
		// Use client.print() to send that string to the server.
        client.print(phant.post());
        delay(1000);
		// Now we'll do some simple checking to see what (if any) response
		// the server gives us.
        while (client.available())
        {
            char c = client.read();
            Serial.print(c);	// Print the response for debugging help.
            if (i < 512)
                response[i++] = c; // Add character to response string
        }
		// Search the response string for "200 OK", if that's found the post
		// succeeded.
        if (strstr(response, "200 OK"))
        {
            oled.print("Success");
            oled.display();
            retVal = 1;
        }
        else if (strstr(response, "400 Bad Request"))
        {	// "400 Bad Request" means the Phant POST was formatted incorrectly.
			// This most commonly ocurrs because a field is either missing,
			// duplicated, or misspelled.
            oled.print("Bad req");
            oled.display();
            retVal = -1;
        }
        else
        {
			// Otherwise we got a response we weren't looking for.
            retVal = -2;
        }
    }
    else
    {	// If the connection failed, print a message:
        oled.print("Connect");
        oled.setCursor(0, 17);
        oled.print("failed");
        oled.display();
        retVal = -3;
    }
    client.stop();	// Close the connection to server.
    delay(1000);    // Allow user to see message on OLED
    
    return retVal;	// Return error (or success) code.
}