#ifndef SMC100C_h
#define SMC100C_h

#include <stdint.h>
#include "serial.h"

class SMC100C {
 public:
  typedef void( *FinishedListener )();
  //ASCII commands from SMC100C User Manual p. 22-70
  enum class CommandType {
    None,
    Acceleration,
    BacklashComp,
    HysterisisComp,
    DriverVoltage,
    KdLowPassFilterCutOff,
    FollowingErrorLim,
    FrictionComp,
    HomeSearchType,
    StageIdentifier,
    LeaveJoggingState,
    KeypadEnable,
    JerkTime,
    DerivativeGain,
    IntegralGain,
    ProportionalGain,
    VelocityFeedForward,
    Enable,
    HomeSearchVelocity,
    HomeSearch,
    HomeSearchTimeout,
    MoveAbs,
    MoveRel,
    MoveEstimate,
    Configure,
    Analog,
    TTLInputVal,
    Reset,
    RS485Adress,
    TTLOutputVal,
    ControlLoopState,
    NegativeSoftwareLim,
    PositiveSoftwareLim,
    StopMotion,
    EncoderIncrementVal,
    CommandErrorString,
    LastCommandErr,
    PositionAsSet,
    PositionReal,
    ErrorStatus,
    Velocity,
    BaseVelocity,
    ControllerRevisionInfo,
    AllConfigParam,
    ESPStageConfig,
  };
  enum class CommandParameterType {
    None,
    Int,
    Float,
  };
  enum class CommandGetSetType {
    None,
    Get,
    Set,
    GetSet,
    GetAlways,
  };
  enum class ModeType {
    Inactive,
    Idle,
    WaitingForCommandReply,
  };
  struct CommandStruct {
    CommandType Command;
    const char* CommandChar;
    CommandParameterType SendType;
    CommandGetSetType GetSetType;
  };
  struct CommandEntry {
    const CommandStruct* Command;
    CommandGetSetType GetOrSet;
    float Parameter;

  };
  enum class StatusType {
    Unknown,
    Error,
    Config,
    NoReference,
    Homing,
    Moving,
    Ready,
    Disabled,
    Jogging,
  };
  struct StatusCharSet {
    const char* Code;
    StatusType Type;
  };

  bool SMC100CInit(const char*);
  void SMC100CClose();
  bool Home(void);
  bool QueryHardware();
  void SetVelocity(float VelocityToSet);
  void RelativeMove(float CommandParameter);
  void AbsoluteMove(float AbsoluteDistanceToMove);
  std::string GetPosition();
  std::string GetCustom(const std::string& Command);
  std::string GetVelocity();
  std::string GetAcceleration();
  std::string GetPositiveLimit();
  std::string GetNegativeLimit();
  std::string GetCurrentStatus();
  const char* GetError();
  char* GetMotionTime();
  void StopMotion();
  void SetPositiveLimit(float Limit);
  void SetNegativeLimit(float Limit);
  void SetAcceleration(float AccelerationToSet);
  void SetJerkTime(float JerkTime);
  int Available();
  //serialib serial;

 private:
     int readString(serial::Serial& my_serial, char* receivedString, char finalChar, unsigned int maxNbBytes, unsigned int timeOut_ms);
  static const char GetCharacter;
  //void Home(void);
  static const CommandStruct CommandLibrary[];
  static const StatusCharSet StatusLibrary[];
  const char* ConvertToErrorString(char ErrorCode);
  bool SendCurrentCommand();
  CommandEntry CommandToPrint;
  const CommandStruct* CurrentCommand;
  CommandGetSetType CurrentCommandGetOrSet;
  float CurrentCommandParameter;
  void SetCommand(CommandType Type, float Parameter, CommandGetSetType GetOrSet);
  StatusType ConvertStatus(char* StatusChar);


  char* SerialRead() {
      static char receivedString[64]; // Adjust size as needed
      char finalChar = '\n';
      unsigned int maxNbBytes = sizeof(receivedString) - 1; // Buffer size minus space for null terminator
      int readStatus;

      readStatus = readString(my_serial, receivedString, finalChar, maxNbBytes, 20); // Pass the serial object

      // Handle different cases based on readStatus
      if (readStatus > 0) {
          return receivedString;
      }
      else if (readStatus == 0) {
          return (char*)"Timeout";
      }
      else {
          return (char*)"Error";
      }
  }
  
  serial::Serial my_serial;
};

#endif
