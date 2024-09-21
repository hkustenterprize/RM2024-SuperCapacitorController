#include "Config.hpp"
#include "IncrementalPID.hpp"

namespace IncrementalPID
{
IncrementalPID::IncrementalPID(float augKPOnTarget, float augKPOnMeasurement, float augKI, float augKD)
    : kPOnTarget(augKPOnTarget),
      kPOnMeasurement(augKPOnMeasurement),
      kI(augKI),
      kD(augKD),
      lastMeasurement(0.0f),
      lastLastMeasurement(0.0f),
      lastTarget(0.0f),
      deltaOutput(0.0f)
{
    configASSERT(this->kPOnTarget >= 0.0f);
    configASSERT(this->kPOnMeasurement >= 0.0f);
    configASSERT(this->kI >= 0.0f);
    configASSERT(this->kD >= 0.0f);
}

void IncrementalPID::update(float target, float measurement)
{
    this->deltaOutput = this->kPOnTarget * (target - this->lastTarget) - this->kPOnMeasurement * (measurement - this->lastMeasurement) +
                        this->kI * (target - measurement) + this->kD * (measurement - 2.0f * this->lastMeasurement + this->lastLastMeasurement);
    this->lastTarget          = target;
    this->lastLastMeasurement = this->lastMeasurement;
    this->lastMeasurement     = measurement;
}

void IncrementalPID::updateDataNoOutput(float target, float measurement)
{
    this->lastTarget          = target;
    this->lastLastMeasurement = this->lastMeasurement;
    this->lastMeasurement     = measurement;
}

float IncrementalPID::getDeltaOutput() const { return this->deltaOutput; }

float IncrementalPID::getDeltaI() const { return this->kI * (this->lastTarget - this->lastMeasurement); }

void IncrementalPID::reset()
{
    this->lastTarget      = 0.0f;
    this->lastMeasurement = 0.0f;
    this->deltaOutput     = 0.0f;
}

}  // namespace IncrementalPID