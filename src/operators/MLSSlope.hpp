#ifndef __ENVIRE__MLS_SLOPE_HPP__
#define __ENVIRE__MLS_SLOPE_HPP__

#include <envire/Core.hpp>

namespace envire
{
    /** This operator computes local slopes on a MLS map
     *
     * It acts on an MLSGrid and updates a Grid<float> with the maximum local
     * slope angles in radians
     *
     * The default operation will compute the maximum slope between the topmost
     * surfaces of a MLS. In practice, it means that it works only on MLS that
     * have one patch per cell.
     *
     * It can be customized by subclassing and overloading the computeGradient
     * operator
     */
    class MLSSlope : public Operator
    {
	ENVIRONMENT_ITEM( MLSSlope )
        double corrected_step_threshold;
        bool use_stddev;

    public:
        MLSSlope()
            : corrected_step_threshold(0.25)
            , use_stddev(false) {}
        MLSSlope(double corrected_step_threshold, bool use_stddev) 
            : corrected_step_threshold(corrected_step_threshold)
            , use_stddev(use_stddev) {}
	void serialize( Serialization &so ) { Operator::serialize( so ) ;}
	void unserialize( Serialization &so ) { Operator::unserialize( so ) ;}

        double computeGradient(double mean0, double mean1, double stdev0, double stdev1);
	bool updateAll();
    };
}

#endif
