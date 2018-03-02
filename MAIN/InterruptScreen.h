#pragma once
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#include "UTFTMenu.h"
#include "InterruptHandler.h"
#include "Drawing.h"
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// экран с графиком прерываний
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
typedef Vector<Point> Points;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
class InterruptScreen : public AbstractTFTScreen
{
  public:

  static AbstractTFTScreen* create();  

  void setList(const InterruptTimeList& list, uint8_t listNum);
  void showChart();
    
protected:

    virtual void doSetup(TFTMenu* menu);
    virtual void doUpdate(TFTMenu* menu);
    virtual void doDraw(TFTMenu* menu);
    virtual void onButtonPressed(TFTMenu* menu, int pressedButton);

private:
      InterruptScreen();

      InterruptTimeList list1;
      InterruptTimeList list2;
      InterruptTimeList list3;

      unsigned long startSeenTime;

    void drawChart(); // рисуем график
    void computeChart(); // пересчитываем график

    Points serie1;
    Points serie2;
    Points serie3;

    void computeSerie(InterruptTimeList& timeList,Points& serie, uint16_t xOffset, uint16_t yOffset);
    void drawSerie(Points& serie,RGBColor color);
  
  
};
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern InterruptScreen* ScreenInterrupt;
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------
