/**
* C S O U N D   V S T 
*
* A VST plugin version of Csound, with Python scripting.
*
* L I C E N S E
*
* This software is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This software is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this software; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "Random.hpp"

namespace csound
{
	boost::mt19937 Random::mersenneTwister;

	template<typename T> void cleanup(T *&t)
	{
		if(t)
		{
			delete t;
			t = 0;
		}
	}

	Random::Random() : 
	row(0), 
		column(Event::HOMOGENEITY), 
		eventCount(0), 
		incrementTime(true),
		uniform_smallint(0),
		uniform_int(0),
		uniform_01(0),
		uniform_real(0),
		bernoulli_distribution(0),
		geometric_distribution(0),
		triangle_distribution(0),
		exponential_distribution(0),
		normal_distribution(0),
		lognormal_distribution(0),
		minimum(0),
		maximum(1),
		q(1),
		a(0),
		b(1),
		c(0),
		Lambda(1),
		mean(0),
		sigma(1)	
	{
		distribution = "uniform_real";
	}

	Random::~Random()
	{
		cleanup(uniform_smallint);
		cleanup(uniform_int);
		cleanup(uniform_01);
		cleanup(uniform_real);
		cleanup(bernoulli_distribution);
		cleanup(geometric_distribution);
		cleanup(triangle_distribution);
		cleanup(exponential_distribution);
		cleanup(normal_distribution);
		cleanup(lognormal_distribution);
	}

	void Random::createDistribution(std::string distribution)
	{
		if(distribution == "uniform_smallint")
		{
			cleanup(uniform_smallint);
			uniform_smallint = new boost::uniform_smallint<boost::mt19937>(mersenneTwister, int(minimum), int(maximum));
		}
		else if(distribution == "uniform_int")
		{
			cleanup(uniform_int);
			uniform_int = new boost::uniform_int<boost::mt19937>(mersenneTwister, int(minimum), int(maximum));
		}
		else if(distribution == "uniform_01")
		{
			cleanup(uniform_01);
			uniform_01 = new boost::uniform_01<boost::mt19937>(mersenneTwister);
		}
		else if(distribution == "uniform_real")
		{
			cleanup(uniform_real);
			uniform_real = new boost::uniform_real<boost::mt19937>(mersenneTwister, minimum, maximum);
		}
		else if(distribution == "bernoulli")
		{
			cleanup(bernoulli_distribution);
			bernoulli_distribution = new boost::bernoulli_distribution<boost::mt19937>(mersenneTwister, q);
		}
		else if(distribution == "geometric")
		{
			cleanup(geometric_distribution);
			geometric_distribution = new boost::geometric_distribution<boost::mt19937>(mersenneTwister, q);
		}
		else if(distribution == "triangle")
		{
			cleanup(triangle_distribution);
			triangle_distribution = new boost::triangle_distribution<boost::mt19937>(mersenneTwister, a, b, c);
		}
		else if(distribution == "exponential")
		{
			cleanup(exponential_distribution);
			exponential_distribution = new boost::exponential_distribution<boost::mt19937>(mersenneTwister, Lambda);
		}
		else if(distribution == "normal")
		{
			cleanup(normal_distribution);
			normal_distribution = new boost::normal_distribution<boost::mt19937>(mersenneTwister, mean, sigma);
		}
		else if(distribution == "lognormal")
		{
			cleanup(lognormal_distribution);
			lognormal_distribution = new boost::lognormal_distribution<boost::mt19937>(mersenneTwister, mean, sigma);
		}
	}
	double Random::sample() const
	{
		if(uniform_smallint) return (*uniform_smallint)();
		if(uniform_int) return (*uniform_int)();
		if(uniform_01) return (*uniform_01)();
		if(uniform_real) return (*uniform_real)();
		if(bernoulli_distribution) return (*bernoulli_distribution)();
		if(geometric_distribution) return (*geometric_distribution)();
		if(triangle_distribution) return (*triangle_distribution)();
		if(exponential_distribution) return (*exponential_distribution)();
		if(normal_distribution) return (*normal_distribution)();
		if(lognormal_distribution) return (*lognormal_distribution)();
		return 0;
	}
	ublas::matrix<double> Random::getLocalCoordinates() const
	{
		ublas::matrix<double> transformation = localCoordinates;
		for(int i = 0; i < Event::HOMOGENEITY; i++)
		{
			transformation[i][Event::HOMOGENEITY] *= sample();
		}
		return transformation;		
	}
	void Random::produceOrTransform(Score &score, size_t beginAt, size_t endAt, const ublas::matrix<double> &globalCoordinates)
	{
		createDistribution(distribution);
		if(eventCount > 0)
		{
			double currentTime = 0;
			for (int i = 0; i < eventCount; i++)
			{
				//  Resample for every generated note.
				ublas::matrix<double> localCoordinates = getLocalCoordinates();
				ublas::matrix<double> compositeCoordinates = ublas::prod(localCoordinates, globalCoordinates);
				Event event(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1);
				Event transformedEvent = ublas::prod(compositeCoordinates, event);
				if (incrementTime)
				{
					double buffer = fabs(transformedEvent.getTime());
					transformedEvent.setTime(buffer + currentTime);
					currentTime += buffer;
				}
				score.push_back(transformedEvent);
			}
		}
		else
		{
			for (size_t i = beginAt; i < endAt; i++)
			{
				ublas::matrix<double> local = getLocalCoordinates();
				ublas::matrix<double> compositeCoordinates = ublas::prod(local, globalCoordinates);
				score[i] = ublas::prod(compositeCoordinates, score[i]);
			}
		}
	}

	void Random::seed(int s)
	{
		//mersenneTwister.seed((unsigned int) s);
	}
}

