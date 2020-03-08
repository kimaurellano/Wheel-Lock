#include "User.h"

User::User(){}

void User::setPhoneNumber(String phoneNumber){
  _phoneNumber = phoneNumber;
}

void User::setSlot(int slot){
  _slot = slot;
}

void User::setStateOfUse(int state){
  _stateOfUse = state;
}

String User::getPhoneNumber(){
  return _phoneNumber;
}

int User::getSlot(){
  return _slot;
}

int User::getStateOfUse(){
  return _stateOfUse;
}