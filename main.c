


#include <stdint.h>
#include "inc/tm4c123gh6pm.h"

// Define traffic lights and pedestrian signals
#define RED_SOUTH    0x08  // PF3 (South Red)
#define YELLOW_SOUTH 0x10  // PF4 (South Yellow)
#define GREEN_SOUTH  0x02  // PE1 (South Green)

#define RED_WEST     0x04  // PF2 (West Red)
#define YELLOW_WEST  0x02  // PF1 (West Yellow)
#define GREEN_WEST   0x01  // PF0 (West Green)

#define WALK_LED     0x04  // PE2 (Pedestrian Walk LED)
#define DONT_WALK_LED 0x08 // PE3 (Pedestrian Don't Walk LED)

#define SOUTH_SENSOR 0x02  // PB1 (South Sensor)
#define WEST_SENSOR  0x04  // PA2 (West Sensor)
#define PED_SENSOR   0x01  // PB0 (Pedestrian Sensor)

#define RED_NORTH   0x08  // PF5 (North Red)
#define YELLOW_NORTH 0x10  // PF6 (North Yellow)
#define GREEN_NORTH  0x02  // PE4 (North Green)

#define RED_EAST    0x08  // PF7 (East Red)
#define YELLOW_EAST 0x10  // PF8 (East Yellow)
#define GREEN_EAST  0x02  // PE5 (East Green)


// Function Prototypes
void PortF_Init(void);
void PortB_Init(void);
void PortA_Init(void);
void PortE_Init(void);
void delayMs(uint32_t ms);
void TrafficControl(void);
void PedestrianControl(void);
void FlashDontWalk(void);
void ServeSouthRoad(void);
void ServeWestRoad(void);

int main(void)
{
    PortF_Init();  // Initialize PortF for traffic lights and pedestrian signals
    PortB_Init();  // Initialize PortB for sensor buttons
    PortA_Init();  // Initialize PortA for West sensor
    PortE_Init();  // Initialize PortE for South Green and pedestrian signals

    while (1)
    {
        TrafficControl();
    }
}

// Initialize PortF for traffic lights
void PortF_Init(void)
{
    SYSCTL_RCGCGPIO_R |= 0x20;  // Enable clock to PortF
    while ((SYSCTL_PRGPIO_R & 0x20) == 0);  // Wait for PortF to be ready

    // Unlock PF0 for use as GPIO
    GPIO_PORTF_LOCK_R = 0x4C4F434B;  // Unlock GPIO Port F
    GPIO_PORTF_CR_R |= 0x01;         // Allow changes to PF0
    GPIO_PORTF_LOCK_R = 0;           // Relock the GPIO port register

    GPIO_PORTF_DIR_R |= 0x1F;  // Set PF0-PF4 as output (West and South lights)
    GPIO_PORTF_DEN_R |= 0x1F;  // Enable digital function on PF0-PF4
}

// Initialize PortB for South and Pedestrian sensor inputs
void PortB_Init(void)
{
    SYSCTL_RCGCGPIO_R |= 0x02;  // Enable clock to PortB
    while ((SYSCTL_PRGPIO_R & 0x02) == 0);  // Wait for PortB to be ready
    GPIO_PORTB_DIR_R &= ~0x03;  // Set PB0 and PB1 as inputs (South, Pedestrian sensors)
    GPIO_PORTB_DEN_R |= 0x03;   // Enable digital function on PB0, PB1
    GPIO_PORTB_PUR_R |= 0x03;   // Enable internal pull-up resistors for PB0, PB1
}

// Initialize PortA for West sensor input
void PortA_Init(void)
{
    SYSCTL_RCGCGPIO_R |= 0x01;  // Enable clock to PortA
    while ((SYSCTL_PRGPIO_R & 0x01) == 0);  // Wait for PortA to be ready
    GPIO_PORTA_DIR_R &= ~WEST_SENSOR;  // Set PA2 as input for West sensor
    GPIO_PORTA_DEN_R |= WEST_SENSOR;   // Enable digital function on PA2
    GPIO_PORTA_PUR_R |= WEST_SENSOR;   // Enable internal pull-up for PA2 (West sensor)
}

// Initialize PortE for South Green and pedestrian signals
void PortE_Init(void)
{
    SYSCTL_RCGCGPIO_R |= 0x10;  // Enable clock to PortE
    while ((SYSCTL_PRGPIO_R & 0x10) == 0);  // Wait for PortE to be ready
    GPIO_PORTE_DIR_R |= 0x0E;   // Set PE1, PE2, PE3 as output (South Green, Pedestrian LEDs)
    GPIO_PORTE_DEN_R |= 0x0E;   // Enable digital function on PE1, PE2, PE3
}

// Main traffic control logic to handle South, West, and Pedestrian signals
void TrafficControl(void)
{
    uint32_t southActive = GPIO_PORTB_DATA_R & SOUTH_SENSOR;
    uint32_t westActive = GPIO_PORTA_DATA_R & WEST_SENSOR;
    uint32_t pedActive = GPIO_PORTB_DATA_R & PED_SENSOR;

    if (southActive && westActive && pedActive) {
        // All three inputs active: Cycle South -> Pedestrian -> West
        ServeSouthRoad();
        PedestrianControl();
        ServeWestRoad();
    }
    else if (southActive && pedActive) {
        // South + Pedestrian active: Alternate South and Pedestrian
        ServeSouthRoad();
        PedestrianControl();
    }
    else if (westActive && pedActive) {
        // West + Pedestrian active: Alternate West and Pedestrian
        ServeWestRoad();
        PedestrianControl();
    }
    else if (southActive && westActive) {
        // South + West active: Alternate South and West roads
        ServeSouthRoad();
        ServeWestRoad();
    }
    else if (southActive) {
        // Only South road active
        ServeSouthRoad();
    }
    else if (westActive) {
        // Only West road active
        ServeWestRoad();
    }
    else if (pedActive) {
        // Only Pedestrian active
        PedestrianControl();
    }
    else {
        // No inputs active: Safe state, serve South road by default
        ServeSouthRoad();
    }
}

// Function to handle pedestrian signal (Walk, Don't Walk)
void PedestrianControl(void)
{
    // Turn on Walk LED, both South and West lights red
    GPIO_PORTE_DATA_R = WALK_LED;
    GPIO_PORTF_DATA_R = RED_SOUTH | RED_WEST;
    delayMs(5000);  // Allow pedestrians to walk for 5 seconds

    // Flash Don't Walk LED before stopping pedestrians
    FlashDontWalk();

    // Turn on Don't Walk LED, continue red on both roads
    GPIO_PORTE_DATA_R = DONT_WALK_LED;
    delayMs(2000);  // Keep Don't Walk for 2 seconds
}

// Flash the Don't Walk LED twice
void FlashDontWalk(void)
{
    int i;
    for (i = 0; i < 2; i++) {
        GPIO_PORTE_DATA_R &= ~DONT_WALK_LED;  // Turn off Don't Walk LED
        delayMs(500);
        GPIO_PORTE_DATA_R |= DONT_WALK_LED;   // Turn on Don't Walk LED
        delayMs(500);
    }
}

// Serve South road traffic: Green -> Yellow -> Red
void ServeSouthRoad(void)
{
    // Green light for South road (PE1)
    GPIO_PORTE_DATA_R = GREEN_SOUTH;
    GPIO_PORTF_DATA_R = RED_WEST;
    delayMs(5000);  // Green for 5 seconds

    // Yellow light for South road (PF4)
    GPIO_PORTF_DATA_R = YELLOW_SOUTH | RED_WEST;
    GPIO_PORTE_DATA_R &= ~GREEN_SOUTH;  // Turn off South Green
    delayMs(2000);  // Yellow for 2 seconds

    // Red light for South road (PF3), all lights red
    GPIO_PORTF_DATA_R = RED_SOUTH | RED_WEST;
    delayMs(1000);  // Red overlap for safety
}

// Serve West road traffic: Green -> Yellow -> Red
void ServeWestRoad(void)
{
    // Green light for West road (PF0)
    GPIO_PORTF_DATA_R = GREEN_WEST | RED_SOUTH;
    delayMs(5000);  // Green for 5 seconds

    // Yellow light for West road (PF1)
    GPIO_PORTF_DATA_R = YELLOW_WEST | RED_SOUTH;
    delayMs(2000);  // Yellow for 2 seconds

    // Red light for West road (PF2), all lights red
    GPIO_PORTF_DATA_R = RED_WEST | RED_SOUTH;
    delayMs(1000);  // Red overlap for safety
}

// Simple delay function
void delayMs(uint32_t ms)
{
    uint32_t i, j;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < 3180; j++) {
            // Do nothing, just waste  time
        }
    }
}
