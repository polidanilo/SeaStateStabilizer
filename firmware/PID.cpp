#include "PID.h"

PIDController::PIDController(double p, double i, double d) {
  kp = p;
  ki = i;
  kd = d;
}

double PIDController::compute(double setpoint, double measured_value, double dt) {
  if (dt <= 0.0) {
    return 0.0;
  }

  double error = setpoint - measured_value;

  // Proportional
  double proportional = kp * error;

  // Integral with simple anti-windup clamp
  integral += error * dt;
  const double integral_limit = 1000.0;  // tune if needed
  if (integral > integral_limit) {
    integral = integral_limit;
  } else if (integral < -integral_limit) {
    integral = -integral_limit;
  }
  double integral_term = ki * integral;

  // Derivative (on error)
  double derivative = (error - previous_error) / dt;
  double derivative_term = kd * derivative;

  double result = proportional + integral_term + derivative_term;

  previous_error = error;

  return result;
}
