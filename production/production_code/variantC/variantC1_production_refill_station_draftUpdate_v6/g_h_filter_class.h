#ifndef _g_h_filter_class_h
#define _g_h_filter_class_h

/*
   ~~~~~Prerequisites~~~~~

   See Github readme

   ~~~~~Details~~~~~~~~~~~~

   Initial Author: Ben Money-Coomes
   Date: 30/10/20
   Purpose: General class for a g-h filter as described by: https://github.com/rlabbe/Kalman-and-Bayesian-Filters-in-Python
   References: See Github Readme

   ~~~~Version Control~~~~~

   v1.0 - [Checkpoint] This version is working with the companion example

*/


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Class Includes.                                                                *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

//pass

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  Start of Class.                                                                *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


class g_h_filter
{
  public:
    //-------- Public variables for g-h filter
    g_h_filter(float, float, float, float, float);
    float measurment; // 'measurement' is the last measurement from the system process
    float x0; //'x0' is the initial value for our state variable
    float dx; //'dx' is the initial change rate for our state variable
    float g; //'g' is the g-h's g scale factor
    float h; //'h' is the g-h's h scale factor
    long dt; //'dt' is the length of the time step between the filter updates

    //-------- Public function definitions for g-h filter
    float return_value(float); // Returns a filtered value when provided with a measurement
    void reset(float, float, float, float, float);// used to reset the filter back to the initial values

  private:
    //-------- Private variables for g-h filter
    float x_pred; // 'x_pred' is the prediction for the state of the system (from the prediction step)
    float x_est; // 'x_est' is the estimate for the state of the system (from the update step)

    float x0_reset; // These are based on the initial filter and are used if the filter is reset
    float dx_reset; 
    float g_reset; 
    float h_reset; 
    long dt_reset; 

    bool first_dt_calculation = true; // a boolean to keep track of whether it is first time dt is calculated
    unsigned long time_right_now; // to keep track of time
    unsigned long time_at_last_step; // to record time at last step

    //-------- Private function definitions for g-h filter
    float make_prediction(); // predict step
    float make_update(float); // update step
    float calc_timestep(); // calculate the timestep used in the prediction function
    
    
    


};

//-------- Class constructor

g_h_filter::g_h_filter(float initial_x0, float initial_dx, float initial_dt, float initial_g, float initial_h)
{
  // Set initial values for relevant variables
  x0 = initial_x0;
  dx = initial_dx;
  g = initial_g;
  h = initial_h;
  dt = initial_dt;

  // Initialise values for filter
  x_est = x0;

  //-------- Below this line saves the reset values for the filter
  x0_reset = x0;
  dx_reset = dx;
  g_reset = g;
  h_reset = h;
  dt_reset = dt;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  This is where we define the three functions that make up the g-h filter        *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


//~~~~~~~~~~~~~~~~ g-h filter -> MAKE PREDICTION FUNCTION ~~~~~~~~~~~~~~~~~~~~~~~~

float g_h_filter:: make_prediction()
{
  // prediction step
  x_pred = x_est + (dx * dt);

  return x_pred; // return the prediction of the system state
}


//~~~~~~~~~~~~~~~~ g-h filter MAKE UPDATE FUNCTION ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

float g_h_filter:: make_update(float my_measurement)
{
  // initialise local variables
  float residual;
  float z = my_measurement;

  // update step
  residual = z - x_pred;
  dx = dx + h * (residual) / dt;
  x_est = x_pred + g * residual;

//  Serial.print("residual, z, dx = ");
//  Serial.print(residual);
//  Serial.print(", ");
//  Serial.print(z);
//  Serial.print(", ");
//  Serial.println(dx);

  return x_est; // return the filtered estimate of the system state
}

//~~~~~~~~~~~~~~~~ g-h filter -> RETURN VALUE FUNCTION ~~~~~~~~~~~~~~~~~~~~~~~~~~

float g_h_filter:: return_value(float my_measurement)
{
  calc_timestep();
  x_pred = make_prediction();
  x_est = make_update(my_measurement);

//  Serial.print("dt, xpred, xest = ");
//  Serial.print(dt);
//  Serial.print(", ");
//  Serial.print(x_pred);
//  Serial.print(", ");
//  Serial.println(x_est);
  
  return x_est;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
  These are helper functions                                                     *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


float g_h_filter:: calc_timestep()
{
  // Check if it is the first time this function is run
  if (first_dt_calculation == true) {
    dt = dt; // set to initial_dt
    time_at_last_step = millis();
    first_dt_calculation = false;
  }
  else {
    time_right_now = millis();
    // this next line catches an error in case the millis() function loops back to zero after 50 days
    if (time_right_now - time_at_last_step >= 0) {
      dt = time_right_now - time_at_last_step;
    }
    else {
      dt = dt; // else keep value the same
    }
    time_at_last_step = millis();
  }
  return dt; // return the calculated timestep
}

void g_h_filter:: reset(float x0_reset, float dx_reset, float dt_reset, float g_reset, float h_reset)
{
  // Set reset values for relevant variables
  x0 = x0_reset;
  dx = dx_reset;
  g = g_reset;
  h = h_reset;
  dt = dt_reset;

  // Initialise values for filter
  x_est = x0;

  // Reset time calc
  first_dt_calculation = true;
}


#endif _g_h_filter_class_h
