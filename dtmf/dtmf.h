// -----------------------------------------------------------------
// dtmf teletype - a spimple teletype appliance mc based
// Copyright (C) 2021  Gabriele Bonacini
//
// This program is free software for no profit use; you can redistribute 
// it and/or modify it under the terms of the GNU General Public License 
// as published by the Free Software Foundation; either version 2 of 
// the License, or (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
// A commercial license is also available for a lucrative use.
// -----------------------------------------------------------------

#pragma once

class DtmfDecoder {
  public:
      DtmfDecoder(void)                      noexcept;
      void readChars(void)                   noexcept;

  private:
      static const size_t      SERIAL_SPEED { 9600 }; 

      const int                Q1        { 2 },       
                               Q2        { 3 },         
                               Q3        { 4 },         
                               Q4        { 5 },
                               STQ       { 6 },
                               STATUS    { A0 },
                               ACTIVITY  { A1 };  

      const char lookup[16];
                               
      bool parity(unsigned char ch)            noexcept;
      bool isPrintable(unsigned char ch)       noexcept;
      int  checkBits(unsigned char par1, 
                     unsigned char par2,
                     unsigned char par3)       noexcept;
      
};

DtmfDecoder::DtmfDecoder(void) noexcept
    : lookup{'D', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '*', '#', 'A', 'B', 'C'}
{
    pinMode(Q1,  INPUT);
    pinMode(Q2,  INPUT);
    pinMode(Q3,  INPUT);
    pinMode(Q4,  INPUT);

    pinMode(STQ, INPUT);

    Serial.begin(SERIAL_SPEED);
}

bool DtmfDecoder::parity(unsigned char ch) noexcept{
          int    count { 0 };

          if(ch & 0x01) count++;
          if(ch & 0x02) count++;
          if(ch & 0x04) count++;
          if(ch & 0x08) count++;
          if(ch & 0x10) count++;
          if(ch & 0x20) count++;
          if(ch & 0x40) count++;
          if(ch & 0x80) count++;

          return (count & 0x01) ? true : false;
}

bool DtmfDecoder::isPrintable(unsigned char ch) noexcept{
    if(ch >= 0x20 && ch <= 0x7E){
                  return true;
    } else {
         switch(ch){
              case 0x09:
              case 0x0A:
              case 0x0B:
              case 0x0C:
              case 0x0D:
                   return true;
         }
     }

     return false;
}

int DtmfDecoder::checkBits(unsigned char par1, unsigned char par2, unsigned char par3) noexcept{
    int check { 0 };
             
    if(parity(par3)) check += 0b00000100;          
    if(parity(par2)) check += 0b00000010;
    if(parity(par1)) check += 0b00000001;  

    return check;
}    

void DtmfDecoder::readChars(void) noexcept{
  unsigned char   curr        { 0 },
                  prev        { 0 },
                  part[3]     { 0, 0, 0 },
                  character   { 0 },
                  parityCode  { 0 };
          size_t  idx         { 0 };
    unsigned int  luxLow      { 0 },
                  luxHigh     { 160 };

          analogWrite(STATUS, luxHigh);
       
  while(true){        
      if(digitalRead(STQ)==HIGH){    
          delay(50);
          curr = 0;
          if(digitalRead(Q1)==HIGH)   curr  = curr  + 0x01;
          if(digitalRead(Q2)==HIGH)   curr  = curr  + 0x02;
          if(digitalRead(Q3)==HIGH)   curr  = curr  + 0x04;
          if(digitalRead(Q4)==HIGH)   curr  = curr  + 0x08;

          switch(curr){
              case 0x0:
                  if(prev == 0)
                       continue; 
                  prev = 0;   
                  analogWrite(ACTIVITY, luxHigh);    
              break;
              case 0x1:
              case 0x2:
              case 0x3:
              case 0x4:
              case 0x5: 
              case 0x6:
              case 0x7:
              case 0x8:
              case 0x9:
              case 0xA:
                  if(prev != 0)
                       continue; 
                  switch(idx){
                    case 0:
                    case 1:
                    case 2:
                         part[idx] = (curr != 0x0A ? curr : 0 );
                         idx++;
                    break;
                    case 3:
                         parityCode = (curr != 0x0A ? curr : 0 );
                         idx++;
                    break;
                    default:
                         Serial.print("Idx Err: ");  
                         Serial.println(idx );
                         idx       = 0;
                         prev      = 0;
                         curr      = 0;  
                         character = 0;
                  }
                  prev  =  curr;
                  analogWrite(ACTIVITY, luxLow);  
              break;   
              case 0x0E: 
                    if(prev != 0x0E){
                            if( checkBits(part[0], part[1], part[2]) ==  parityCode ){
                                  character = part[0] * 100 + part[1] * 10 + part[2];
                                  if(isPrintable(character))
                                      Serial.print(static_cast<char>(character));
                                  else
                                      Serial.print("<B>");
                            }else{
                                  Serial.print("<E>");
                            }
                    }
                    character  =  0;
                    idx        =  0;
                    prev       =  0x0E;
                    analogWrite(ACTIVITY, luxLow);  
             break;
             case 0x0C: 
                    if(prev != 0x0C){
                          Serial.println("");
                          Serial.println("");
                          Serial.println("");
                    }
                    character  =  0;
                    idx        =  0;
                    prev       =  0x0C;
                    analogWrite(ACTIVITY, luxLow);  
             break; 
             default:   
                    Serial.print("<U>");
                    character  =  0;
                    idx        =  0;
                    prev       =  0;
                    analogWrite(ACTIVITY, luxLow);           
          }     
     }
  }
}
