#ifndef GaRep_CALIBRATE_H_
#define GaRep_CALIBRATE_H_

#include "daisy.h"

using namespace daisy;

namespace GaRep
{
enum CalibrationProcedureState
{
    PATCH_1V,
    PATCH_3V,
    PATCH_DONE_CALIBRATING,
};

class Calibrate
{
  public:
    VoctCalibration cal;
    Calibrate() {}
    ~Calibrate() {}

    bool         ProcessCalibration(float &cv_in, bool &button_pressed);
    inline void  Start() { cal_state_ = PATCH_1V; };
    inline float GetBrightness() { return brightness_; };

  private:
    CalibrationProcedureState cal_state_{PATCH_DONE_CALIBRATING};
    float                     value_1v_; /**< Temporary value for 1V */
    float                     brightness_;
};

} // namespace GaRep

#endif // GaRep_CALIBRATE_H_