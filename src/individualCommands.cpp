
#include <SFML/Graphics.hpp>
#include "SMC100C.h"
#include "LibUSB3DPrinter.h" // Include the provided header file
#include <serial.h>
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <thread>
#include <iostream>
#include "individualCommands.h"
#include <vector>
#include <string>
#include <future>
#include <filesystem>
#include <fstream>
#include <regex>
#include <fstream>
#include <sstream>
#include <tuple>
#include <unordered_map>

namespace fs = std::filesystem;

/**************************************************************************************************************************************
Function:
    initializeController
Parameters:
    SMC100C& controller: Reference to an SMC100C controller object
Returns:
    bool: True if initialization succeeds, False otherwise
Description:
    Initializes the motorized stage controller with the communication port specified by the global variable `globalComPort`.
    The function first locks the mutex to ensure thread-safe access to `globalComPort`, then converts the QString value of
    `globalComPort` to a const char* format suitable for the SMC100CInit function. It attempts to initialize the controller
    with this COM port and prints the result to the console.
Notes:
    - The function assumes that `globalComPort` holds the correct COM port identifier.
    - Requires `QMutexLocker` to synchronize access to the `globalComPort` variable, ensuring thread safety.
    - Uses `QByteArray::data()` to convert `QString` to `const char*`, which is necessary for `SMC100CInit`.
    - If initialization fails, the function suggests exiting the program due to lack of control over the controller.
Author:
    Mats Grobe, 28/02/2024
***************************************************************************************************************************************/

bool initializeController(SMC100C& controller) {
    // Ensure thread-safe access to globalComPort
    QString comPort;
    {
        QMutexLocker locker(&mutexForComPort);
        comPort = globalComPort;
    }

    // Convert QString to const char* for SMC100CInit
    QByteArray comPortArray = comPort.toLocal8Bit();
    const char* comPortCStr = comPortArray.data();

    std::cout << "Testing Initialization with " << comPortCStr << "... ";
    if (controller.SMC100CInit(comPortCStr)) {
        std::cout << "Success" << std::endl;
        return true; // Initialization succeeded
    }
    else {
        std::cout << "Initialization Failed! Exit Program. No control!" << std::endl;
        return false; // Initialization failed
    }
}
/**************************************************************************************************************************************
Function:
    checkStage
Parameters:
    None
Returns:
    StageStatus: A struct containing the current status of the stage, including position, velocity, acceleration, and limit values.
Description:
    This function attempts to initialize the motorized stage controller and then queries it for its current status, including
    position, velocity, acceleration, and positive/negative limits. It performs a homing operation before retrieving the values.
    If initialization fails, the program exits. The function assumes that the controller's responses follow a specific format,
    and it extracts numerical values from these responses for the status struct. If any operation fails or an exception occurs,
    the function logs the error but attempts to continue gracefully, returning whatever status information was successfully gathered.
Notes:
    - This function assumes that `SMC100C::Home`, `GetPosition`, `GetVelocity`, `GetAcceleration`, `GetPositiveLimit`, and
      `GetNegativeLimit` return strings in a specific format, from which numerical values can be extracted.
    - Uses `std::remove` and `std::stof` for string manipulation and conversion to float, respectively.
    - Any exceptions caught during the retrieval of stage parameters are logged, and the function attempts to continue,
      potentially returning partial or default status information.
    - Exiting the program on initialization failure indicates that the function considers successful controller communication
      critical for further operations.
Author:
    Mats Grobe, 28/02/2024
***************************************************************************************************************************************/


StageStatus checkStage() {

    SMC100C controller;

    if (!initializeController(controller)) {

        exit(0);
    }
    
    
    

    // Test Home
    std::cout << "Testing Home... ";
    if (controller.Home()) {
        std::cout << "Success" << std::endl;
    }
    else {
        std::cout << "Failed" << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Wait for response

    StageStatus status;

    // Assuming GetPosition, GetVelocity, etc., return strings like "Pos:123.45"
    try {
        std::string pos = controller.GetPosition();
        pos.erase(std::remove(pos.begin(), pos.end(), '\n'), pos.end()); // Remove newline
        pos.erase(std::remove(pos.begin(), pos.end(), '\r'), pos.end()); // Remove carriage return
        std::cout << "Position: " << pos.substr(3, 6) << " mm" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Wait for response
        std::string vel = controller.GetVelocity();
        vel.erase(std::remove(vel.begin(), vel.end(), '\n'), vel.end()); // Remove newline
        vel.erase(std::remove(vel.begin(), vel.end(), '\r'), vel.end()); // Remove carriage return
        std::cout << "Velocity: " << vel.substr(3, 2) << " mm/s" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Wait for response

        std::string acc = controller.GetAcceleration();
        acc.erase(std::remove(acc.begin(), acc.end(), '\n'), acc.end()); // Remove newline
        acc.erase(std::remove(acc.begin(), acc.end(), '\r'), acc.end()); // Remove carriage return
        std::cout << "Acceleration: " << acc.substr(3, 2) << " mm/s2" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Wait for response

        std::string pL = controller.GetPositiveLimit();
        pL.erase(std::remove(pL.begin(), pL.end(), '\n'), pL.end()); // Remove newline
        pL.erase(std::remove(pL.begin(), pL.end(), '\r'), pL.end()); // Remove carriage return
        std::cout << "Positive Limit: " << pL.substr(3, 2) << " mm" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Wait for response

        std::string nL = controller.GetNegativeLimit();
        nL.erase(std::remove(nL.begin(), nL.end(), '\n'), nL.end()); // Remove newline
        nL.erase(std::remove(nL.begin(), nL.end(), '\r'), nL.end()); // Remove carriage return
        std::cout << "Negative Limit: " << nL.substr(3, 2) << " mm" << std::endl;

        // Extract float values from the string using std::stof and substring operations
        // Note: Ensure the substring indices and lengths are correct for your data format
        status.position = std::stof(pos.substr(3, 6)); // Adjust according to actual format
        status.velocity = std::stof(vel.substr(3, 2)); // Adjust according to actual format
        status.acceleration = std::stof(acc.substr(3, 2)); // Adjust according to actual format
        status.positiveLimit = std::stof(pL.substr(3, 2)); // Adjust according to actual format
        status.negativeLimit = std::stof(nL.substr(3, 2)); // Adjust according to actual format
    }
    catch (const std::exception& e) {
        std::cerr << "Exception occurred in checkStage: " << e.what() << std::endl;
        // Handle error, possibly set status fields to default/error values
    }

    return status;

}

/**************************************************************************************************************************************
Function:
    checkStageDummy
Parameters:
    None
Returns:
    StageStatus: A struct filled with dummy values for stage parameters.
Description:
    This function provides a simulated version of checkStage by returning a StageStatus struct populated with predefined dummy values.
    It is used for testing or when the actual hardware is not accessible. The function sets predefined values for position, velocity,
    acceleration, and positive/negative limits that mimic realistic stage parameters but do not reflect any real-world device's status.
Notes:
    This function is purely for simulation purposes and does not interact with actual hardware.
    Useful in scenarios where the actual device communication is not feasible, for example, during unit testing or UI development.
Author:
    Mats Grobe, 28/02/2024
***************************************************************************************************************************************/
// Dummy function simulating checkStage()
StageStatus checkStageDummy() {
    StageStatus status;

    // Assign some dummy values
    status.position = 123.45f;
    status.velocity = 67.89f;
    status.acceleration = 10.11f;
    status.positiveLimit = 200.0f;
    status.negativeLimit = -10.0f;

    return status;
}


/**************************************************************************************************************************************
Function:
    getLightEngineStatus
Parameters:
    None
Returns:
    LightEngineStatus: A struct containing the current status of the light engine, including status codes, current, system status,
    default LED status, and temperature.
Description:
    Queries the light engine for its current operational status, including general status, current, system status, LED default status,
    and temperature. This function assumes that appropriate initialization and setup have been done and calls various APIs to populate
    the returned struct with current values from the light engine.
Notes:
    - The function assumes that the light engine is accessible and that its APIs, such as GetStatus, GetCurrent, GetSysStatus, etc.,
      are available and return data in expected formats.
    - Error handling for API calls or data conversion is not explicitly mentioned but should be considered.
Author:
    Mats Grobe, 28/02/2024
***************************************************************************************************************************************/
// Real function to get the status from the light engine
LightEngineStatus getLightEngineStatus() {
    LightEngineStatus status;

    // Assuming you have setup code to initialize and work with your light engine
    // Here, just call the APIs and assign the values to your structure
    status.status = GetStatus();
    GetCurrent(0, &status.current); // Assuming index 0 for simplicity
    status.sysStatus = GetSysStatus();

    unsigned char flag;
    if (GetLedDefaultStatus(&flag)) {
        status.ledDefaultStatus = flag ? true : false;
    }

    int16_t temp;
    if (GetTemperature(&temp)) {
        status.temperature = temp;
    }

    return status;
}
/**************************************************************************************************************************************
Function:
    getLightEngineStatusDummy
Parameters:
    None
Returns:
    LightEngineStatus: A struct filled with dummy values for light engine parameters.
Description:
    Provides a simulated version of getLightEngineStatus by returning a LightEngineStatus struct populated with predefined dummy
    values. This function is used for testing or when the actual light engine hardware is not accessible, providing a way to develop
    and test software without the need for actual hardware.
Notes:
    This function is intended for simulation and testing purposes, offering a convenient way to proceed with development in the absence
    of physical hardware or during early development stages.
Author:
    Mats Grobe, 28/02/2024
***************************************************************************************************************************************/
// Dummy function to simulate getting the status from the light engine
LightEngineStatus getLightEngineStatusDummy() {
    LightEngineStatus status;

    // Assign some dummy values
    status.status = 0x01; // Example status
    status.current = 0x05; // Example current value
    status.sysStatus = 0x02; // Example system status
    status.ledDefaultStatus = true; // Example LED status
    status.temperature = 25; // Example temperature in Celsius

    return status;
}

/**************************************************************************************************************************************
Function:
    TurnLightEngineOn
Parameters:
    None
Returns:
    None
Description:
    Initiates the process of turning the light engine on by checking the connectivity with USB devices, selecting a device based on
    predefined criteria (e.g., index), and sending a command to power on the light engine. It includes checks for device availability,
    online status, and monitors the system status until the light engine is fully operational or a timeout occurs.
Notes:
    - The function assumes the use of specific APIs for USB device enumeration, selection, and status checking.
    - It demonstrates a pattern of waiting for a condition (light engine ready) within a timeout period, which is critical for operations
      requiring confirmation of device readiness.
Author:
    Mats Grobe, 28/02/2024
***************************************************************************************************************************************/

void TurnLightEngineOn() {

    std::cout << "Checking connectivity with the Light Engine..." << std::endl;

    // Enumerate USB devices and check if any is available
    unsigned char numDevices = EnumUsbDevice();
    if (numDevices == 0) {
        std::cerr << "No USB devices found." << std::endl;
    }
    else {
        std::cout << "Number of USB devices found: " << static_cast<int>(numDevices) << std::endl;
    }

    // Assuming you know the index of your device (here, using 0 as an example)
    unsigned char deviceIndex = 0;
    SetUsbDeviceIndex(deviceIndex);

    // Check if the USB device is online
    unsigned char isOnline = CheckUSBOnline();
    if (isOnline == 0) { // Assuming '0' indicates the device is not online
        std::cerr << "USB device is not online." << std::endl;
    }
    else {
        std::cout << "USB device is online." << std::endl;
    }

    unsigned char result;

    S16 temp, time;
    unsigned char stat = 4;
    unsigned char status;

    // Example: Turn on the power (assuming '1' is for power on)
    status = PowerOnOff(1);
    if (status == 1) { // Assuming '0' indicates success
        std::cout << "Power turned on successfully." << std::endl;
    }
    else {
        std::cout << "Failed to turn on power." << std::endl;
    }


    char userChoice; // User's choice for turning the LED on or off
    unsigned char flag; // Flag for the LedOnOff function
    std::cout << "System is warming up..."<<std::endl;

    auto frameEndTime = std::chrono::high_resolution_clock::now();
    auto inittime = std::chrono::high_resolution_clock::now();
    std::chrono::seconds maxDuration(600); // 100ms for the dark time

    while (stat != 1) {
        stat = GetSysStatus();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << static_cast<int>(stat) << std::endl;

        if (stat != 4 && stat != 1) {
            std::cout << static_cast<int>(stat) << std::endl;
        }

        auto timeouttime = std::chrono::high_resolution_clock::now();
        auto total = std::chrono::duration_cast<std::chrono::seconds>(timeouttime - inittime);
        //std::cout << "Time: " << total.count() << " s" << std::endl;

        if (total > maxDuration) {
            status = PowerOnOff(0);
            std::cout << "Took tooooooooooo long..." << std::endl;
            if (status == 1) { // Assuming '0' indicates success
                std::cout << "Power turned off successfully." << std::endl;
            }
            else {
                std::cerr << "Failed to turn off power." << std::endl;
            }
            exit(1); // Exit the entire program
        }
    }

}


/**************************************************************************************************************************************
Function:
    TurnLightEngineOff
Parameters:
    None
Returns:
    None
Description:
    Sends a command to the light engine to power off. This function is straightforward in its approach, directly invoking the command
    to turn off the power and handling the success or failure of this operation based on the command's return value.
Notes:
    - This function is a counterpart to TurnLightEngineOn, providing a simple method to safely power down the light engine.
    - The success of the operation is logged, offering basic feedback on the action's outcome.
Author:
    Mats Grobe, 28/02/2024
***************************************************************************************************************************************/

void TurnLightEngineOff() {
    unsigned char status;
    // Example: Turn off the power (assuming '0' is for power off)
    status = PowerOnOff(0);
    if (status == 1) { // Assuming '0' indicates success
        std::cout << "Power turned off successfully." << std::endl;
    }
    else {
        std::cout << "Failed to turn off power." << std::endl;
    }
}

/**************************************************************************************************************************************
Function:
    customSort
Parameters:
    const std::string& a, const std::string& b: Two strings representing file paths to be compared.
Returns:
    bool: True if 'a' should come before 'b' based on the custom sorting criteria; otherwise, false.
Description:
    A utility function designed to compare two file paths based on a specific sorting criterion, useful for organizing image files in
    a sequence. The function extracts numerical values from the file names using a regular expression and compares these values to
    determine the sort order.
Notes:
    - Assumes file names contain numerical identifiers that dictate their sort order, captured using a regular expression.
    - This function is particularly useful when dealing with a series of image files that need to be processed or displayed in a
      specific sequence.
Author:
    Mats Grobe, 28/02/2024
***************************************************************************************************************************************/

bool customSort(const std::string& a, const std::string& b) {
    std::regex rgx("SEC_(\\d+)\\.PNG");
    std::smatch matchA, matchB;

    if (std::regex_search(a, matchA, rgx) && std::regex_search(b, matchB, rgx)) {
        return std::stoi(matchA[1]) < std::stoi(matchB[1]);
    }
    return a < b; // Fallback to default sort if regex fails
}
/**************************************************************************************************************************************
Function:
    moveStage
Parameters:
    SMC100C& controller, double stepSize, bool isClip, float dlpPumpingAction
Returns:
    void
Description:
    Moves the stage according to specified parameters, handling movements for both upward and downward directions based on the
    dlpPumpingAction value. The function ensures the stage reaches the desired positions by continually checking the controller's
    status until it reports 'Ready', indicating completion of the move. The process incorporates error handling to manage exceptions
    during movement commands, with retries as necessary.
Notes:
    - The function showcases robust control and error management strategies in handling precise stage movements.
    - Designed to support conditional logic for dlpPumpingAction, accommodating different operational modes (e.g., isClip), which
      allows for flexible stage manipulation tailored to specific experimental requirements.
    - Utilizes a lambda function, moveAndCheckReady, to encapsulate the repetitive task of moving the stage and verifying readiness,
      enhancing code readability and maintainability.
Author:
    Mats Grobe, 28/02/2024
***************************************************************************************************************************************/


void moveStage(SMC100C& controller, double stepSize, bool isClip, float dlpPumpingAction) {
    auto sleepDuration = std::chrono::milliseconds(3);

    auto moveAndCheckReady = [&](float moveStep) {
        while (true) {
            try {
                controller.RelativeMove(moveStep);
                std::this_thread::sleep_for(sleepDuration); // Allow time for movement to initiate

                // Continuously check if the controller is ready
                while (true) {
                    std::this_thread::sleep_for(sleepDuration); // Regular checks for the "Ready" status
                    if (controller.GetCurrentStatus() == "Ready") {
                        return; // Exit the loop if controller is ready
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Exception caught: " << e.what() << ". Retrying..." << std::endl;
                // Optionally, handle the error more specifically or log it
                std::this_thread::sleep_for(sleepDuration); // Wait before retrying
            }
        }
    };

    if (!isClip) {
        std::cout << "DLP Movement triggered UP" << std::endl;
        float updlp = -1 * dlpPumpingAction;
        moveAndCheckReady(updlp); // Move up
    }

    moveAndCheckReady(stepSize); // Move to the next position

    if (!isClip) {
        moveAndCheckReady(dlpPumpingAction); // Move down
    }
}
/**************************************************************************************************************************************
Function:
    checkTimeout
Parameters:
    const std::chrono::high_resolution_clock::time_point& startTime, const std::chrono::seconds& timeout
Returns:
    bool: True if the current time exceeds the specified timeout relative to the start time, false otherwise.
Description:
    Calculates the elapsed time from a given start time and checks if it has exceeded a specified timeout duration. If the elapsed time
    is greater than or equal to the timeout, the function returns true, indicating that the timeout has been reached.
Notes:
    - This utility function is useful for implementing time-bound operations, allowing for early termination or timeout checks in
      long-running processes.
    - The function uses high-resolution clock for precise time measurement, suitable for timing operations where accuracy is important.
Author:
    Mats Grobe, 28/02/2024
***************************************************************************************************************************************/


bool checkTimeout(const std::chrono::high_resolution_clock::time_point& startTime, const std::chrono::seconds& timeout) {
    auto currentTime = std::chrono::high_resolution_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime) >= timeout) {
        std::cout << "Timeout reached." << std::endl;
        return true; // Timeout reached
    }
    return false; // No timeout
}

/**************************************************************************************************************************************
Function:
    waitForPosition
Parameters:
    SMC100C& controller, float targetPosition, float tolerance, const std::chrono::seconds& timeout
Returns:
    void
Description:
    Periodically queries the controller for the current position and waits until it matches the target position within a specified
    tolerance. If the position does not match within the tolerance before the timeout is reached, the function exits.
Notes:
    - Implements a retry mechanism with a delay between checks to avoid overwhelming the controller with requests.
    - Uses exception handling to manage potential errors during controller communication, ensuring robustness in the presence of
      transient communication issues.
Author:
    Mats Grobe, 28/02/2024
***************************************************************************************************************************************/

// Function to wait for the position to match the target within a tolerance
void waitForPosition(SMC100C& controller, float targetPosition, float tolerance, const std::chrono::seconds& timeout) {
    auto startTime = std::chrono::high_resolution_clock::now();
    bool positionMatched = false;
    while (!positionMatched) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Wait before checking again

        try {
            std::string pos = controller.GetPosition();
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Additional wait

            pos.erase(std::remove(pos.begin(), pos.end(), '\n'), pos.end());
            pos.erase(std::remove(pos.begin(), pos.end(), '\r'), pos.end());

            if (pos.length() >= 9) { // Check if pos string is long enough
                std::cout << "Position: " << pos.substr(3, 6) << " mm" << std::endl;
                float currentPosition = std::stof(pos.substr(3, 6)); // Safely convert string to float

                if (abs(currentPosition - targetPosition) < tolerance) {
                    positionMatched = true;
                }
            }
            else {
                std::cerr << "Position string format invalid or too short." << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Exception caught while processing position: " << e.what() << std::endl;
            // Handle error without terminating the loop
        }
        controller.AbsoluteMove(targetPosition);

        if (checkTimeout(startTime, timeout)) break; // Check for timeout
    }
}

/**************************************************************************************************************************************
Function:
    waitForVelocity
Parameters:
    SMC100C& controller, float targetVelocity, float tolerance, const std::chrono::seconds& timeout
Returns:
    void
Description:
    Monitors the controller's velocity and waits for it to reach a specified target velocity within a certain tolerance. The function
    repeatedly checks the velocity and compares it to the target, exiting either when the match is achieved or the operation times out.
Notes:
    - Similar to waitForPosition, this function includes periodic checks with sleep intervals and robust error handling.
    - Critical for operations requiring precise control over motion parameters, ensuring the system reaches the desired state before
      proceeding.
Author:
    Mats Grobe, 28/02/2024
***************************************************************************************************************************************/

// Function to wait for the velocity to match the target within a tolerance
void waitForVelocity(SMC100C& controller, float targetVelocity, float tolerance, const std::chrono::seconds& timeout) {
    auto startTime = std::chrono::high_resolution_clock::now();
    bool velocityMatched = false;
    while (!velocityMatched) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Wait before checking again

        try {
            std::string vel = controller.GetVelocity();
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Additional wait

            vel.erase(std::remove(vel.begin(), vel.end(), '\n'), vel.end());
            vel.erase(std::remove(vel.begin(), vel.end(), '\r'), vel.end());

            if (vel.length() >= 2) { // Check if vel string is long enough
                std::cout << "Velocity: " << vel.substr(3, 2) << " mm/s" << std::endl;
                float currentVelocity = std::stof(vel.substr(3, 2)); // Safely convert string to float

                if (abs(currentVelocity - targetVelocity) < tolerance) {
                    velocityMatched = true;
                }
            }
            else {
                std::cerr << "Velocity string format invalid or too short." << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Exception caught while processing velocity: " << e.what() << std::endl;
            // Handle error without terminating the loop
        }
        controller.SetVelocity(targetVelocity);

        if (checkTimeout(startTime, timeout)) break; // Check for timeout
    }
}


/**************************************************************************************************************************************
Function:
    InitializeSystem
Parameters:
    int inputCurrent, float initialPosition, float velocity, sf::RenderWindow& window, float initialVelocity
Returns:
    void
Description:
    Sets up the initial state of the system including the stage and the graphical display. Initializes the controller, configures
    stage velocity and position, and prepares the SFML window for rendering. This function encapsulates the initial setup required
    before starting main operations or experiments.
Notes:
    - This function demonstrates integration between hardware control (stage movement) and software (graphical interface setup),
      providing a complete system initialization routine.
    - Detailed error checking and handling ensure that the system is correctly initialized and ready for operation.
Author:
    Mats Grobe, 28/02/2024
***************************************************************************************************************************************/

void InitializeSystem(int inputCurrent, float initialPosition, float velocity, sf::RenderWindow& window, float initialVelocity ) {
    // Retrieve the desktop mode of the primary monitor
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();


    // Enable VSync to synchronize with the monitor refresh rate
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(30);

    //--------------------------------------------------------Stage Set Up-----------------------------------------------------------------

    SMC100C controller;
    if (!initializeController(controller)) {
        exit(0);
    }

    // Test Home
    std::cout << "Testing Home... ";
    if (controller.Home()) {
        std::cout << "Success" << std::endl;
    }
    else {
        std::cout << "Failed" << std::endl;
    }

    std::cout << "Input Current: " << inputCurrent << std::endl;


    float intermediatePosition = initialPosition - 10.0f;
    float positionTolerance = 0.01f;
    float velocityTolerance = 0.5f;
    auto timeoutSeconds = std::chrono::seconds(60); // Example: 3 seconds timeout
    auto startTime = std::chrono::high_resolution_clock::now();

    // 1. Set the initial velocity
    controller.SetVelocity(initialVelocity);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait before checking again
    waitForVelocity(controller, initialVelocity, velocityTolerance, timeoutSeconds);

    // 2. Move to the intermediate position
    controller.AbsoluteMove(intermediatePosition);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait before checking again
    waitForPosition(controller, intermediatePosition, positionTolerance, timeoutSeconds);

    // 3. Change the velocity to the final velocity
    controller.SetVelocity(velocity);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait before checking again
    waitForVelocity(controller, velocity, velocityTolerance, timeoutSeconds);

    // 4. Move to the initial position
    controller.AbsoluteMove(initialPosition);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait before checking again
    waitForPosition(controller, initialPosition, positionTolerance, timeoutSeconds);




    std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Wait for response
    //---------------------------------------------------------------------------------Stage Setup-------------------------------------------------------------------------

    U8 currentValue; // = GetCurrent(0, 0);



    window.clear();
    window.display();
    SetCurrent(0, static_cast<U8>(inputCurrent));

    GetCurrent(0, &currentValue);
    std::cout << "The  current value is: " << static_cast<int>(currentValue) << std::endl;

}

/**************************************************************************************************************************************
Function:
    DeinitializeSystem
Parameters:
    int inputCurrent, float initialPosition, float velocity, sf::RenderWindow& window, float initialVelocity
Returns:
    void
Description:
    Safely shuts down the system by resetting the stage to a base position and turning off any initialized hardware or software
    components. This function reverses the setup done by InitializeSystem, ensuring that the system is left in a safe state after
    operations are completed.
Notes:
    - The deinitialization process is critical for ensuring that hardware is not left in an uncertain state, which could lead to
      problems on subsequent runs.
    - Includes careful handling of stage movement back to a safe position and ensures the graphical window is properly closed.
Author:
    Mats Grobe, 28/02/2024
***************************************************************************************************************************************/

void DeinitializeSystem(int inputCurrent, float initialPosition, float velocity, sf::RenderWindow& window, float initialVelocity) {
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Settle for 50 ms


    SMC100C controller;
    if (!initializeController(controller)) {
        exit(0);
    }

    // Test Home
    std::cout << "Testing Home... ";
    if (controller.Home()) {
        std::cout << "Success" << std::endl;
    }
    else {
        std::cout << "Failed" << std::endl;
    }
    float position = 0.0f; // Initialize position with a default value outside the try block

    try {
        std::string pos = controller.GetPosition();
        pos.erase(std::remove(pos.begin(), pos.end(), '\n'), pos.end()); // Remove newline
        pos.erase(std::remove(pos.begin(), pos.end(), '\r'), pos.end()); // Remove carriage return
        position = std::stof(pos.substr(3, 6));
    }
    catch (const std::exception& e) {
        std::cerr << "Error converting position string to float: " << e.what() << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Wait for response

    float intermediatePosition = position - 10.0f;
    float positionTolerance = 0.01f;
    float velocityTolerance = 0.5f;
    auto timeoutSeconds = std::chrono::seconds(60); // Example: 3 seconds timeout
    auto startTime = std::chrono::high_resolution_clock::now();



    // 2. Move to the intermediate position
    controller.AbsoluteMove(intermediatePosition);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Wait for response
    waitForPosition(controller, intermediatePosition, positionTolerance, timeoutSeconds);

    // 3. Change the velocity to the fast velocity
    controller.SetVelocity(initialVelocity);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Wait for response
    waitForVelocity(controller, initialVelocity, velocityTolerance, timeoutSeconds);

    // 4. Move to the base position
    controller.AbsoluteMove(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Wait for response
    waitForPosition(controller, 0, positionTolerance, timeoutSeconds);




    std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Wait for response
    //---------------------------------------------------------------------------------Stage Setup-------------------------------------------------------------------------

    U8 currentValue; // = GetCurrent(0, 0);



    window.clear();
    window.display();
    SetCurrent(0, static_cast<U8>(inputCurrent));

    GetCurrent(0, &currentValue);
    std::cout << "The  current value is: " << static_cast<int>(currentValue) << std::endl;

}

/**************************************************************************************************************************************
Function:
    RunFull
Parameters:
    const std::string& directoryPath, int maxImageDisplayCount, float stepSize, int mindarktime, sf::RenderWindow& window, LogCallback logCallback, std::function<bool()> getAbortFlag, bool isClip, float dlpPumpingAction, int initialExposureCounter, int initialLayers
Returns:
    void
Description:
    Executes a full run of the system, cycling through a sequence of images while controlling the stage based on specified parameters.
    This function integrates image display, stage control, and dynamic parameter adjustments into a cohesive operation, demonstrating
    the system's full capabilities.
Notes:
    - The function is designed to be flexible, allowing for dynamic adjustment of parameters such as exposure time and step size.
    - It showcases the ability to respond to external signals (e.g., an abort flag) for increased control during operation.
Author:
    Mats Grobe, 28/02/2024
***************************************************************************************************************************************/

void RunFull(const std::string& directoryPath, int maxImageDisplayCount, float stepSize, int mindarktime, sf::RenderWindow& window, LogCallback logCallback, std::function<bool()> getAbortFlag, bool isClip,
    float dlpPumpingAction, 
    int initialExposureCounter, int initialLayers) {


    logCallback("Run Full has started");
    if (!isClip) {
        logCallback("Dlp mode initialized.");
    }
    //--------------------------------------------------------Graphics Set Up-----------------------------------------------------------------

    // Retrieve the desktop mode of the primary monitor
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

    // Enable VSync to synchronize with the monitor refresh rate
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(30);

    // Vector to store image paths
    std::vector<std::string> imagePaths;


    logCallback("Adding paths to imagePaths vector.");

    // Iterate over files in the directory and add image paths to the vector
    try {
        for (const auto& entry : fs::directory_iterator(directoryPath)) {
            imagePaths.push_back(entry.path().string());
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
        logCallback("Filesystem error: " + std::string(e.what()));

        // Handle error (e.g., return or throw an exception)
        return;
    }


    std::sort(imagePaths.begin(), imagePaths.end(), customSort);

    sf::Texture currentTexture, nextTexture, textures[2];
    sf::Sprite sprite;
    int currentTextureIndex = 0; // Index to track the current texture

    size_t currentImage = 0, nextImage = 1, currentImageIndex = 0;
    bool showImage = true, nextImageLoaded = false, isNextImageLoading = false, allImagesShown = false;

    // Preload the first image
    if (!textures[currentTextureIndex].loadFromFile(imagePaths[currentImage])) {
        std::cerr << "Failed to load first image: " << imagePaths[currentImage] << std::endl;
        logCallback("Failed to load first image: " + imagePaths[currentImage]);

        //return -1;
    }

    // Pre-calculate the scale for all sprites
    float scaleX = static_cast<float>(window.getSize().x) / textures[currentTextureIndex].getSize().x;
    float scaleY = static_cast<float>(window.getSize().y) / textures[currentTextureIndex].getSize().y;



    //--------------------------------------------------------Stage Set Up-----------------------------------------------------------------

    SMC100C controller;

    // Test Initialization
    logCallback("Testing Initialization...");
    if (!initializeController(controller)) {
        exit(0);
    }
    logCallback("Success");
    

    std::future<void> stageThread; // Future for async operation

    bool isStageThreadRunning = false;


    std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Wait for response


    //---------------------------------------------------------------------------------Stage Setup-------------------------------------------------------------------------



    std::chrono::high_resolution_clock::time_point darkTimeStart;
    std::chrono::milliseconds darkDuration(mindarktime); // 100ms for the dark time


    // Initialize timers
    std::chrono::high_resolution_clock::time_point lastFrameStart = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point lastFrameEnd = lastFrameStart;
    std::chrono::high_resolution_clock::time_point phaseStartTime = std::chrono::high_resolution_clock::now(); // Track the start of the phase


    float accumulatedDuration = 0.0f;
    int imageDisplayCount = 0;
    int darkDisplayCount = 0;
    const int maxDarkDisplayCount = 1;
    sf::Clock clock;
    bool filenameLogged = false; // Ensures filename is logged once per image
    bool inLightPhase = true;

    // Start the game loop
    bool displayImage = true;
    bool change = true;
    U8 currentValue; // = GetCurrent(0, 0);
    int currentLayer = 0; // Track the current layer being processed
    bool inInitialPhase = true;
    int maxCount;

    while (window.isOpen()) {
        // Process events
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Draw the white or black screen
        if (displayImage) {


            if (getAbortFlag()) {
                logCallback("Run Full aborted.");
                // Perform any necessary cleanup...
                return; // Exit the function
            }

            // Clear screen
            window.clear();
            sprite.setTexture(textures[currentTextureIndex]);
            sprite.setScale(scaleX, scaleY);
            window.draw(sprite);
            window.display(); // Update the window (this is where VSync wait happens)

            if (!filenameLogged && inLightPhase) {
                logCallback("Displaying: " + imagePaths[currentImageIndex]);
                filenameLogged = true;
                auto now = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - phaseStartTime).count();
                logCallback("Dark phase duration: " + std::to_string(duration) + " ms");
                inLightPhase = false; // Next phase is dark
                phaseStartTime = now; // Reset start time for dark phase
            }

            if (inInitialPhase) {
                maxCount = initialExposureCounter;
            }
            else {
                maxCount = maxImageDisplayCount;
            }

            imageDisplayCount++;
            //std::cout << imageDisplayCount << std::endl;

            if (imageDisplayCount >= maxCount) {
                displayImage = false;
                imageDisplayCount = 0;
                currentLayer++;
                if (currentLayer >= initialLayers) {
                    inInitialPhase = false;
                }

                try {
                    std::string pos = controller.GetPosition();
                    pos.erase(std::remove(pos.begin(), pos.end(), '\n'), pos.end());
                    pos.erase(std::remove(pos.begin(), pos.end(), '\r'), pos.end());
                    if (pos.length() >= 9) { // Ensure the string is long enough for substr operation
                        std::string positionValue = pos.substr(3, 6);
                        std::string posMessage = "Position: " + positionValue + " mm";
                        logCallback(posMessage);
                    }
                    else {
                        logCallback("Error: Position string too short or in unexpected format.");
                    }
                }
                catch (const std::exception& e) {
                    logCallback("Exception caught while processing position: " + std::string(e.what()));
                    // Handle the exception or log it, but don't terminate the loop
                }

                // Debug print the current status
                std::cout << "Current Image Index: " << currentImageIndex << " / " << imagePaths.size() << std::endl;
                logCallback("Current Image Index: " + std::to_string(currentImageIndex) + " / " + std::to_string(imagePaths.size()));
                darkTimeStart = std::chrono::high_resolution_clock::now(); // Start dark time
            }

        }
        else {

            // Display the dark screen
            window.clear();
            // Update the window (this is where VSync wait happens)
            window.display();

            // Immediately after the dark phase display call
            if (!inLightPhase) {
                auto now = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - phaseStartTime).count();
                logCallback("Light phase duration: " + std::to_string(duration) + " ms");
                inLightPhase = true; // Next phase is light
                phaseStartTime = now; // Reset start time for light phase
            }

            if (!isStageThreadRunning && !nextImageLoaded) {
                // Start the stage control thread with the user-defined step size
                stageThread = std::async(std::launch::async, moveStage, std::ref(controller), stepSize, isClip, dlpPumpingAction);
                isStageThreadRunning = true;
            }

            // Start loading the next image 
            if (currentImageIndex + 1 < imagePaths.size() && !isNextImageLoading) {
                isNextImageLoading = true;
                int nextTextureIndex = 1 - currentTextureIndex;
                textures[nextTextureIndex].loadFromFile(imagePaths[currentImageIndex + 1]);
                nextImageLoaded = true;
                std::cout << "Next Image Loaded." << std::endl;
                logCallback("Next Image Loaded.");

            }

            else if (currentImageIndex + 1 >= imagePaths.size() && !isNextImageLoading) {
                allImagesShown = true;
            }

            // Check if the stage thread has completed
            if (isStageThreadRunning && stageThread.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                stageThread.get();
                isStageThreadRunning = false;
            }



            auto currentTime = std::chrono::high_resolution_clock::now();

            if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - darkTimeStart) > darkDuration && nextImageLoaded && !isStageThreadRunning) {
                //std::cout <<"Image Loading Time:" << std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - darkTimeStart) << std::endl;
                displayImage = true;
                darkDisplayCount = 0;
                isNextImageLoading = false;
                currentImageIndex++;
                currentTextureIndex = 1 - currentTextureIndex;
                nextImageLoaded = false;
                int counter = 0;
                filenameLogged = false;

                while (counter <= 1) {
                    // Display the dark screen
                    window.clear();
                    window.display();

                    counter++;
                }
            }


        }

        if (allImagesShown && !displayImage) {
            std::cout << "All images shown, exiting program." << std::endl;
            logCallback("All images shown, exiting program.");
            SetCurrent(0, static_cast<U8>(0));
            break;
        }

    }

    // Ensure the stage thread is finished before exiting
    if (isStageThreadRunning) {
        stageThread.get();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Wait for response

    std::string position;
    position = controller.GetPosition();
    std::cout << "Final position: " << position << std::endl;
    logCallback("Final position: " + position);


    if (controller.Home()) {
        std::cout << "Homed" << std::endl;
        logCallback("Homed");
    }
    else {
        std::cout << "Failed" << std::endl;
    }


    controller.SMC100CClose();
    std::cout << "Closed connection" << std::endl;
    logCallback("Closed connection");

    window.close();

    return;

    logCallback("Run Full has finished");
}



/**************************************************************************************************************************************
Function:
    InitializeSystemDummy
Parameters:
    const std::string& directoryPath, int inputCurrent, float initialPosition, float initialVelocity, sf::RenderWindow& window
Returns:
    void
Description:
    A simplified version of the InitializeSystem function for simulation or testing purposes. It sets up a graphical window and
    simulates loading and displaying images from a specified directory without controlling actual hardware.
Notes:
    - Ideal for development and testing when hardware is not available or for software functionality verification.
    - Focuses on the software side, specifically graphical display aspects, without engaging in hardware control.
Author:
    Mats Grobe, 28/02/2024
***************************************************************************************************************************************/


void InitializeSystemDummy(const std::string& directoryPath, int inputCurrent, float initialPosition, float initialVelocity, sf::RenderWindow& window) {



    // Vector to store image paths
    std::vector<std::string> imagePaths;


    std::cout << "Adding paths to imagePaths vector." << std::endl;

    // Iterate over files in the directory and add image paths to the vector
    try {
        for (const auto& entry : fs::directory_iterator(directoryPath)) {
            imagePaths.push_back(entry.path().string());
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
        // Handle error (e.g., return or throw an exception)
        return;
    }


    std::sort(imagePaths.begin(), imagePaths.end(), customSort);

    sf::Texture currentTexture, nextTexture, textures[2];
    sf::Sprite sprite;
    int currentTextureIndex = 0; // Index to track the current texture

    size_t currentImage = 0, nextImage = 1, currentImageIndex = 0;
    bool showImage = true, nextImageLoaded = false, isNextImageLoading = false, allImagesShown = false;

    // Preload the first image
    if (!textures[currentTextureIndex].loadFromFile(imagePaths[currentImage])) {
        std::cerr << "Failed to load first image: " << imagePaths[currentImage] << std::endl;
        //return -1;
    }

    // Pre-calculate the scale for all sprites
    float scaleX = static_cast<float>(window.getSize().x) / textures[currentTextureIndex].getSize().x;
    float scaleY = static_cast<float>(window.getSize().y) / textures[currentTextureIndex].getSize().y;


    window.clear();
    sprite.setTexture(textures[currentTextureIndex]);
    sprite.setScale(scaleX, scaleY);
    window.draw(sprite);


    // Drawing operations...
    window.clear();
    // Draw stuff
    window.display();

}

/**************************************************************************************************************************************
Function:
    RunFullDummy
Parameters:
    const std::string& directoryPath, sf::RenderWindow& window
Returns:
    void
Description:
    Simulates a full run of the system using a graphical window to display images from a specified directory. This dummy function
    is intended for testing the image display functionality independently of the hardware control aspects of the system.
Notes:
    - Useful for verifying the image handling and display logic in isolation from the rest of the system's operations.
    - Provides a straightforward way to test the graphical user interface components without needing access to the physical setup.
Author:
    Mats Grobe, 28/02/2024
***************************************************************************************************************************************/


void RunFullDummy(const std::string& directoryPath, sf::RenderWindow& window) {
    // Retrieve the desktop mode of the primary monitor
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

    // Enable VSync to synchronize with the monitor refresh rate
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(30);

    // Vector to store image paths
    std::vector<std::string> imagePaths;

    // Load image paths
    try {
        for (const auto& entry : fs::directory_iterator(directoryPath)) {
            if (entry.is_regular_file()) {
                imagePaths.push_back(entry.path().string());
            }
        }
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Error accessing directory: " << e.what() << '\n';
        return;
    }

    // Sort image paths if needed
    std::sort(imagePaths.begin(), imagePaths.end(), customSort); // Assuming customSort is defined elsewhere

    size_t currentImageIndex = 0;
    sf::Texture texture;
    sf::Sprite sprite;

    // Main loop
    while (window.isOpen() && currentImageIndex < imagePaths.size()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Load and display the current image
        if (!texture.loadFromFile(imagePaths[currentImageIndex])) {
            std::cerr << "Failed to load image: " << imagePaths[currentImageIndex] << '\n';
        }
        else {
            sprite.setTexture(texture, true);
            window.clear();
            window.draw(sprite);
            window.display();

            // Move to the next image after displaying the current one
            currentImageIndex++;
            // Simulate some delay between images
            sf::sleep(sf::seconds(1));
        }
    }

    // Optionally, close the window after showing all images
    window.close();
}


/**************************************************************************************************************************************
Function:
    RunFullDynamic
Parameters:
    const std::string& directoryPath, float stepSize, sf::RenderWindow& window, LogCallback logCallback, std::function<bool()> getAbortFlag, bool isClip, float dlpPumpingAction, const std::vector<std::pair<LayerSettings, int>>& orderedSettings
Returns:
    void
Description:
    Enhances the RunFull function with the ability to dynamically adjust printing parameters for each layer. This function iterates
    through a set of predefined layer settings, applying them to control the system's operation in a fine-grained manner.
Notes:
    - Demonstrates advanced usage of the system's capabilities, allowing for complex experiments with varying parameters across layers.
    - The function's design supports experimentation with different settings to optimize outcomes based on dynamic criteria.
Author:
    Mats Grobe, 28/02/2024
***************************************************************************************************************************************/


void RunFullDynamic(const std::string& directoryPath, float stepSize, sf::RenderWindow& window, LogCallback logCallback, std::function<bool()> getAbortFlag, bool isClip,
    float dlpPumpingAction,
    const std::vector<std::pair<LayerSettings, int>>& orderedSettings) {

    for (const auto& pair : orderedSettings) {
        const LayerSettings& settings = pair.first;
        int layerCount = pair.second;

        // Apply settings
        std::cout << "setIntensity:  " << settings.intensity <<
            ", setExposureTime:  " << settings.exposureTime << ", setDarkTime:  " << settings.darkTime << std::endl;

        // Wait for one second after processing a group of layers with the same settings
        std::cout << "Waiting for 1 second after changing parameters..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Print layers with the current settings
        for (int i = 0; i < layerCount; ++i) {
            //Printing operations take place here
            std::cout << "Printed layer with Intensity: " << settings.intensity
                << ", Exposure Time: " << settings.exposureTime
                << ", Dark Time: " << settings.darkTime << std::endl;
        }


    }


    logCallback("Run Full Dynamic has started");
    if (!isClip) {
        logCallback("Dlp mode initialized.");
    }
    //--------------------------------------------------------Graphics Set Up-----------------------------------------------------------------

    // Retrieve the desktop mode of the primary monitor
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

    // Enable VSync to synchronize with the monitor refresh rate
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(30);

    // Vector to store image paths
    std::vector<std::string> imagePaths;


    logCallback("Adding paths to imagePaths vector.");

    // Iterate over files in the directory and add image paths to the vector
    try {
        for (const auto& entry : fs::directory_iterator(directoryPath)) {
            imagePaths.push_back(entry.path().string());
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
        logCallback("Filesystem error: " + std::string(e.what()));

        // Handle error (e.g., return or throw an exception)
        return;
    }


    std::sort(imagePaths.begin(), imagePaths.end(), customSort);

    sf::Texture currentTexture, nextTexture, textures[2];
    sf::Sprite sprite;
    int currentTextureIndex = 0; // Index to track the current texture

    size_t currentImage = 0, nextImage = 1, currentImageIndex = 0;
    bool showImage = true, nextImageLoaded = false, isNextImageLoading = false, allImagesShown = false;

    // Preload the first image
    if (!textures[currentTextureIndex].loadFromFile(imagePaths[currentImage])) {
        std::cerr << "Failed to load first image: " << imagePaths[currentImage] << std::endl;
        logCallback("Failed to load first image: " + imagePaths[currentImage]);

        //return -1;
    }

    // Pre-calculate the scale for all sprites
    float scaleX = static_cast<float>(window.getSize().x) / textures[currentTextureIndex].getSize().x;
    float scaleY = static_cast<float>(window.getSize().y) / textures[currentTextureIndex].getSize().y;



    //--------------------------------------------------------Stage Set Up-----------------------------------------------------------------

    SMC100C controller;

    // Test Initialization
    logCallback("Testing Initialization...");
    if (!initializeController(controller)) {
        exit(0);
    }
    logCallback("Success");

    std::future<void> stageThread; // Future for async operation

    bool isStageThreadRunning = false;


    std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Wait for response


    //---------------------------------------------------------------------------------Stage Setup-------------------------------------------------------------------------



    std::chrono::high_resolution_clock::time_point darkTimeStart;
    


    // Initialize timers
    std::chrono::high_resolution_clock::time_point lastFrameStart = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point lastFrameEnd = lastFrameStart;
    std::chrono::high_resolution_clock::time_point phaseStartTime = std::chrono::high_resolution_clock::now(); // Track the start of the phase


    float accumulatedDuration = 0.0f;
    int imageDisplayCount = 0;
    int darkDisplayCount = 0;
    const int maxDarkDisplayCount = 1;
    sf::Clock clock;
    bool filenameLogged = false; // Ensures filename is logged once per image
    bool inLightPhase = true;

    // Start the game loop
    bool displayImage = true;
    bool change = true;
    U8 currentValue; // = GetCurrent(0, 0);
    int currentLayer = 0; // Track the current layer being processed
    bool inInitialPhase = true;

    auto startPhaseTime = std::chrono::high_resolution_clock::now();
    bool inExposurePhase = true;
    int layercounter = 0;

    while (window.isOpen()) {
        // Process events
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        logCallback("Just before loop.");

        for (const auto& setting : orderedSettings) {
            const LayerSettings& currentSetting = setting.first;
            int layerCount = setting.second;

            // Apply the current settings
            logCallback("Applying settings: Intensity " + std::to_string(currentSetting.intensity) +
                ", Exposure Time: " + std::to_string(currentSetting.exposureTime) +
                ", Dark Time: " + std::to_string(currentSetting.darkTime));

            int maxImageDisplayCount = currentSetting.exposureTime;
            std::chrono::milliseconds darkDuration(currentSetting.darkTime); // 100ms for the dark time
            SetCurrent(0, static_cast<U8>(currentSetting.intensity));

            std::this_thread::sleep_for(std::chrono::milliseconds(2000));  // Wait for response
            layercounter = 0;

            while (layercounter <= layerCount) {

                // Draw the white or black screen
                if (displayImage) {


                    if (getAbortFlag()) {
                        logCallback("Run Full aborted.");
                        // Perform any necessary cleanup...
                        return; // Exit the function
                    }

                    // Clear screen
                    window.clear();
                    sprite.setTexture(textures[currentTextureIndex]);
                    sprite.setScale(scaleX, scaleY);
                    window.draw(sprite);
                    window.display(); // Update the window (this is where VSync wait happens)

                    if (!filenameLogged && inLightPhase) {
                        logCallback("Displaying: " + imagePaths[currentImageIndex]);
                        filenameLogged = true;
                        auto now = std::chrono::high_resolution_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - phaseStartTime).count();
                        logCallback("Dark phase duration: " + std::to_string(duration) + " ms");
                        inLightPhase = false; // Next phase is dark
                        phaseStartTime = now; // Reset start time for dark phase
                    }


                    imageDisplayCount++;
                    //std::cout << imageDisplayCount << std::endl;

                    if (imageDisplayCount >= maxImageDisplayCount) {
                        displayImage = false;
                        imageDisplayCount = 0;
                        currentLayer++;

                        try {
                            std::string pos = controller.GetPosition();
                            pos.erase(std::remove(pos.begin(), pos.end(), '\n'), pos.end());
                            pos.erase(std::remove(pos.begin(), pos.end(), '\r'), pos.end());
                            if (pos.length() >= 9) { // Ensure the string is long enough for substr operation
                                std::string positionValue = pos.substr(3, 6);
                                std::string posMessage = "Position: " + positionValue + " mm";
                                logCallback(posMessage);
                            }
                            else {
                                logCallback("Error: Position string too short or in unexpected format.");
                            }
                        }
                        catch (const std::exception& e) {
                            logCallback("Exception caught while processing position: " + std::string(e.what()));
                            // Handle the exception or log it, but don't terminate the loop
                        }

                        // Debug print the current status
                        std::cout << "Current Image Index: " << currentImageIndex << " / " << imagePaths.size() << std::endl;
                        logCallback("Current Image Index: " + std::to_string(currentImageIndex) + " / " + std::to_string(imagePaths.size()));
                        darkTimeStart = std::chrono::high_resolution_clock::now(); // Start dark time
                    }

                }
                else {

                    // Display the dark screen
                    window.clear();
                    // Update the window (this is where VSync wait happens)
                    window.display();

                    // Immediately after the dark phase display call
                    if (!inLightPhase) {
                        auto now = std::chrono::high_resolution_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - phaseStartTime).count();
                        logCallback("Light phase duration: " + std::to_string(duration) + " ms");
                        inLightPhase = true; // Next phase is light
                        phaseStartTime = now; // Reset start time for light phase
                    }

                    if (!isStageThreadRunning && !nextImageLoaded) {
                        // Start the stage control thread with the user-defined step size
                        stageThread = std::async(std::launch::async, moveStage, std::ref(controller), stepSize, isClip, dlpPumpingAction);
                        isStageThreadRunning = true;
                    }

                    // Start loading the next image 
                    if (currentImageIndex + 1 < imagePaths.size() && !isNextImageLoading) {
                        isNextImageLoading = true;
                        int nextTextureIndex = 1 - currentTextureIndex;
                        textures[nextTextureIndex].loadFromFile(imagePaths[currentImageIndex + 1]);
                        nextImageLoaded = true;
                        std::cout << "Next Image Loaded." << std::endl;
                        logCallback("Next Image Loaded.");

                    }

                    else if (currentImageIndex + 1 >= imagePaths.size() && !isNextImageLoading) {
                        allImagesShown = true;
                    }

                    // Check if the stage thread has completed
                    if (isStageThreadRunning && stageThread.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                        stageThread.get();
                        isStageThreadRunning = false;
                    }



                    auto currentTime = std::chrono::high_resolution_clock::now();

                    if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - darkTimeStart) > darkDuration && nextImageLoaded && !isStageThreadRunning) {
                        //std::cout <<"Image Loading Time:" << std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - darkTimeStart) << std::endl;
                        displayImage = true;
                        darkDisplayCount = 0;
                        isNextImageLoading = false;
                        currentImageIndex++;
                        currentTextureIndex = 1 - currentTextureIndex;
                        nextImageLoaded = false;
                        int counter = 0;
                        filenameLogged = false;

                        while (counter <= 1) {
                            // Display the dark screen
                            window.clear();
                            window.display();

                            counter++;
                        }
                        std::cout << "Printed layer with Intensity: " << currentSetting.intensity
                            << ", Exposure Time: " << currentSetting.exposureTime
                            << ", Dark Time: " << currentSetting.darkTime << std::endl;
                        layercounter++;
                    }




                }
                if (allImagesShown && !displayImage) {
                    std::cout << "All images shown, exiting program." << std::endl;
                    logCallback("All images shown, exiting program.");
                    SetCurrent(0, static_cast<U8>(0));
                    break;
                }
            }


        }

        break;
    }

    // Ensure the stage thread is finished before exiting
    if (isStageThreadRunning) {
        stageThread.get();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Wait for response

    std::string position;
    position = controller.GetPosition();
    std::cout << "Final position: " << position << std::endl;
    logCallback("Final position: " + position);


    if (controller.Home()) {
        std::cout << "Homed" << std::endl;
        logCallback("Homed");
    }
    else {
        std::cout << "Failed" << std::endl;
    }


    controller.SMC100CClose();
    std::cout << "Closed connection" << std::endl;
    logCallback("Closed connection");

    window.close();

    return;

    logCallback("Run Full has finished");
}


/**************************************************************************************************************************************
Function:
    readSettingsOrdered
Parameters:
    const std::string& filePath
Returns:
    std::vector<std::pair<LayerSettings, int>>
Description:
    Reads layer settings from a specified file and organizes them into a vector of pairs, where each pair consists of LayerSettings
    and an integer representing the number of layers to be printed with those settings.
Notes:
    - The function demonstrates data handling and preprocessing necessary for dynamic operation modes, such as RunFullDynamic.
    - It emphasizes the system's adaptability and potential for automated parameter adjustments based on external input.
Author:
    Mats Grobe, 28/02/2024
***************************************************************************************************************************************/

LayerSettings::LayerSettings(int i, int e, int d) : intensity(i), exposureTime(e), darkTime(d) {}


// Function to read and store settings in order of appearance
std::vector<std::pair<LayerSettings, int>> readSettingsOrdered(const std::string& filePath) {
    std::ifstream file(filePath);
    std::string line;
    std::vector<std::pair<LayerSettings, int>> orderedSettings;
    std::map<LayerSettings, int> settingsCount;

    // Skip header
    std::getline(file, line);

    // Read and parse each line
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        int layer, intensity, exposureTime, darkTime;
        char delim; // For the commas

        iss >> layer >> delim >> intensity >> delim >> exposureTime >> delim >> darkTime;
        LayerSettings currentSetting(intensity, exposureTime, darkTime);

        // If this setting is new, add it to the ordered vector
        if (settingsCount.find(currentSetting) == settingsCount.end()) {
            orderedSettings.emplace_back(currentSetting, 1);
        }
        else {
            // If it exists, increment the count in both the map and the vector
            settingsCount[currentSetting]++;
            for (auto& setting : orderedSettings) {
                if (setting.first == currentSetting) {
                    setting.second++;
                    break;
                }
            }
        }

        // Also keep track in the map for easy finding
        settingsCount[currentSetting]++;
    }

    return orderedSettings;
}