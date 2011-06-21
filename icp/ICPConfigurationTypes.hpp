#ifndef __ICP_CONFIGURATION_TYPES__
#define __ICP_CONFIGURATION_TYPES__


namespace envire {
namespace icp
{

    struct ICPConfiguration{
	//maximum number of iterations for the icp algorithm'
	int max_iterations;
	//overlap between model and measurement (between [0..1]).'
	double overlap; 
	//icp will stop if the mean square error is smaller than the given value.'
	double min_mse;
	//icp will stop if the difference in mean square error is smaller
	double min_mse_diff; 
	//'density of the model pointcloud'
	double model_density;
	//'density of the measurement pointcloud
	double measurement_density;
    };

    
    struct SigmaPointConfiguration 
    {

	double min_norm; 
	double max_norm; 
	int n_sigma; 
    }; 
    
    
    struct ClusteringConfiguration{

	//the minimal number of points needed in a cluster 
	uint min_number_of_points; 
	
	bool remove_outliners; 
	
	//defines the limits for removing the outliners 
	SigmaPointConfiguration outliners_position;
	
	SigmaPointConfiguration outliners_orientation;
	
    }; 
    
    enum SAMPLING_MODE {  
	SIGMA_SAMPLING, 
	UNIFORM_SAMPLING
    }; 

    struct SamplingConfiguration{
	
	SAMPLING_MODE mode; 
	
	//defines sampling region
	SigmaPointConfiguration region_sample_position;
	
	SigmaPointConfiguration region_sample_orientation;
	
	#ifndef __orogen 
	    SamplingConfiguration() 
		:mode(UNIFORM_SAMPLING) {}
	#endif
    }; 
    
    struct HistogramConfiguration{
	double histogram_rejection_threshold; 
	double number_bins; 
	double area;
	bool normalization; 
	bool outliners;
	double mean; 
	double sigma; 
    };



}
}
#endif

