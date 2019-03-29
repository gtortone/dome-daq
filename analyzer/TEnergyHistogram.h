#ifndef TEnergyHistogram_h
#define TEnergyHistogram_h

#include <string>
#include "THistogramArrayBase.h"
#include "TSimpleHistogramCanvas.hxx"

/// Class for making histograms of waveform baselines
class TEnergyHistogram : public THistogramArrayBase {

public:
   TEnergyHistogram();

   void UpdateHistograms(TDataContainer& dataContainer);  

   void CreateHistograms();
};

#endif
