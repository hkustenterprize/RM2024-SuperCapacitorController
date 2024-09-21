#pragma once

namespace IncrementalPID
{
class IncrementalPID
{
   public:
    IncrementalPID(float augKPOnTarget, float augKPOnMeasurement, float augKI, float augKD);

    void update(float target, float measurement);

    void updateDataNoOutput(float target, float measurement);

    float getDeltaOutput() const;

    float getDeltaI() const;

    void reset();

   private:
    float kPOnTarget;
    float kPOnMeasurement;
    float kI;
    float kD;

    float lastMeasurement;
    float lastLastMeasurement;
    float lastTarget;

    float deltaOutput;
};
}  // namespace PowerPID