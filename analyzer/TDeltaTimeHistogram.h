#ifndef TDeltaTimeHistogram_h
#define TDeltaTimeHistogram_h

#include <string>
#include "THistogramArrayBase.h"
#include "TSimpleHistogramCanvas.hxx"

class TDeltaTimeHistogram : public THistogramArrayBase {

public:
   TDeltaTimeHistogram();

   void UpdateHistograms(TDataContainer& dataContainer);  

   void CreateHistograms();
};

#endif
