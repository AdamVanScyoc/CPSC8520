#pragma once

#include "RNGObject.h"

#include <math.h>
#include <stdlib.h>
#include <time.h>

class ExpRNGObject : public RNGObject {
	public:
		// Generate an Exponential random variable, grater that min 
		// and less than max. Return as a double.
		double getVariate_d() {
			double lam = 0.05;
			double Xi = 0.0;
			double ret = 0.0;

			do {
				Xi = (rand() / (RAND_MAX + 1.0));
				ret = -log(1 - Xi) / lam + this->min;
			} while(ret < this->min || ret > this->max);

			return ret;
		};

		int getVariate_i() {
			return round(getVariate_d());
		};
};
