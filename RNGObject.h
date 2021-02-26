#pragma once

#include <stdlib.h>
#include <time.h>
#include <math.h>

class RNGObject {
	protected:
	public:
			double min = 0.0, max = 1.0;
			unsigned int seed = 0;

			// Quiz3.pdf says to make "two virtual constructors" but then
			// describes how to implement the constructors in the base class
			// (i.e. non-virtually) so I've made them non-virtual.
			RNGObject() {
				// Seed with current time object.
				seed = time(NULL);

				// Seed the RNG
				srand(seed);
				srand48(seed);
			};

			RNGObject(unsigned int s) {
				// Seed with provided value;
				seed = s;

				// Seed the RNG
				srand(seed);
				srand48(seed);
			};

			virtual double getMin() {
				return min;
			}

			virtual double getMax() {
				return max;
			}

			virtual void setRange(double mi, double ma) {
				// Set range of RNG.
				if (mi < ma && mi >= 0.0) {
					min = mi; 
					max = ma;
				} else {
					min = 0.0;
					max = 1.0;
				}
			};

			virtual int getVariate_i() {
				double Xi = 0.0;

				// Find a random double that is less than min
				// and greater than max.
				//while (Xi = drand48() + (rand() % (int)ceil(max - min)) + 1)
				while ((Xi = lrand48()))
					if (Xi >= min && Xi <= max)
						break;

				// Return value rounded to nearest integer.
				// Note that rounded number is possibly
				// > max or < min.
				return round(Xi);
			};

			virtual double getVariate_d() {
				double Xi;

				// Find a random double that is less than min
				// and greater than max.
				//while (Xi = drand48() + (rand() % (int)ceil(max - min)) + 1)
				while ((Xi = drand48()))
					if (Xi >= min && Xi <= max)
						break;

				// Return value rounded to nearest integer.
				// Note that rounded number is possibly
				// > max or < min.
				return Xi;
			};

			virtual void setSeed(unsigned int s) {
				seed = s;
				srand(seed);
				srand48(seed);
			};

			virtual unsigned int getSeed() {
				return seed;
			}
};
