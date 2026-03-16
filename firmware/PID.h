class PIDController {
  private:
    double kp;
    double ki;
    double kd;

    double previous_error = 0.0;
    double integral = 0.0;

  public:
    PIDController(double p, double i, double d);

    // Compute PID output given setpoint, measurement and elapsed time (seconds)
    double compute(double setpoint, double measured_value, double dt);
};
