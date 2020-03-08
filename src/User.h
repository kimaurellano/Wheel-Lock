#ifndef USER_H
#define USER_H

#include <Arduino.h>

class User {
  private:
    String _phoneNumber;
    int _slot; 
    int _stateOfUse;

  public:
    User();
    
    void setPhoneNumber(String phoneNumber);
    void setSlot(int slot);
    void setStateOfUse(int state);

    String getPhoneNumber();
    int getSlot();
    int getStateOfUse();
};

#endif