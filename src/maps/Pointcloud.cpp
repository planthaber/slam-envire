#include "Core.hpp"
#include "maps/Pointcloud.hpp"
#include "tools/PlyFile.hpp"

#include <fstream>

using namespace envire;

ENVIRONMENT_ITEM_DEF( Pointcloud )

const std::string Pointcloud::VERTEX_COLOR = "vertex_color";
const std::string Pointcloud::VERTEX_NORMAL = "vertex_normal";
const std::string Pointcloud::VERTEX_VARIANCE = "vertex_variance";
const std::string Pointcloud::VERTEX_ATTRIBUTES = "vertex_attributes";

Pointcloud::Pointcloud() : sensor_origin(Eigen::Affine3d::Identity())
{
}

Pointcloud::Pointcloud(const base::samples::Pointcloud &pointcloud) : sensor_origin(Eigen::Affine3d::Identity())
{
    copyFrom(pointcloud);
}


Pointcloud::~Pointcloud()
{
}

void Pointcloud::serialize(Serialization& so)
{
    serialize(so, true);
}

void Pointcloud::serialize(Serialization& so, bool handleMap)
{
    CartesianMap::serialize(so);

    so.write( "sensor_origin", sensor_origin );

    if(handleMap)
	writePly( getMapFileName() + ".ply", so.getBinaryOutputStream(getMapFileName() + ".ply") );
}

void Pointcloud::unserialize(Serialization& so, bool handleMap)
{
    CartesianMap::unserialize(so);

    if( so.hasKey( "sensor_origin" ) )
        so.read("sensor_origin", sensor_origin );
    else
        sensor_origin = Eigen::Affine3d::Identity();

    if(handleMap)
    {
    if( !readPly( getMapFileName() + ".ply", so.getBinaryInputStream(getMapFileName() + ".ply") ) )
        readText( so.getBinaryInputStream(getMapFileName() + ".txt") );
    }
}

bool Pointcloud::writePly(const std::string& filename, std::ostream& os, bool const doublePrecision /* = true */)
{
    PlyFile ply(filename);
    return ply.serialize( this, os, doublePrecision );
}

bool Pointcloud::readPly(const std::string& filename, std::istream& is)
{
    PlyFile ply(filename);
    return ply.unserialize( this, is );
}

bool Pointcloud::writeText(std::ostream& os)
{
    for(size_t i=0;i<vertices.size();i++)
    {
	os << vertices[i].x() << " " << vertices[i].y() << " " << vertices[i].z() << std::endl;
    }
    
    return true;
}

bool Pointcloud::readText(std::istream& is, int sample, TextFormat format)
{
    const int max_line_length = 255;
    std::vector<Eigen::Vector3d> *color = 0;
    if(format == XYZR )
        color = &getVertexData<Eigen::Vector3d>( VERTEX_COLOR );
    while( !is.eof() )
    {
	if( sample == 1 || (rand() % sample == 0) )
	{
	    double x, y, z, c;
	    is >> x >> y >> z;
	    if( format == XYZR && color )
	    {
		is >> c;
		color->push_back( Eigen::Vector3d::Identity() * c / 255.0 );
	    }
	
	    vertices.push_back( Eigen::Vector3d( x,y,z ) );
	    is.ignore( max_line_length, '\n' );
	}
	else
	    is.ignore( max_line_length, '\n' );
        // check for eof bit, to avoid a copy of the last sample
        is.peek();
    }

    return true;
}

Pointcloud* Pointcloud::importCsv(const std::string& file, FrameNode* fn, int sample, TextFormat format)
{
    std::ifstream data(file.c_str());
    if( data.fail() )  
    {
        throw std::runtime_error("Could not open file '" + file + "'.");
    }
    Pointcloud* pc = new Pointcloud();
    pc->readText( data, sample, format );
    data.close();

    Environment* env = fn->getEnvironment();
    env->attachItem(pc);
    pc->setFrameNode(fn);

    return pc;
}

void Pointcloud::copyFrom( Pointcloud* source, bool transform )
{
    // TODO only copies the vertices for now
    clear();

    // get relative transform 
    Transform t = source->getFrameNode()->relativeTransform( getFrameNode() );
    bool needsTransform = !t.isApprox( Transform( Transform::Identity() ) );

    if( !transform || !needsTransform )
    {
	vertices = source->vertices;
    }
    else
    {
	for( std::vector<Eigen::Vector3d>::iterator it = source->vertices.begin(); it != source->vertices.end(); it ++ )
	    vertices.push_back( t * *it );
    }
}

void Pointcloud::copyFrom(const base::samples::Pointcloud& source)
{
    clear();
    // we have to use iterators here because of eigen do not align problem
    vertices.resize(source.points.size());
    std::copy(source.points.begin(),source.points.end(),vertices.begin());

    std::vector<Eigen::Vector3d>& colors(getVertexData<Eigen::Vector3d>(VERTEX_COLOR));
    std::vector<base::Vector4d>::const_iterator iter = source.colors.begin();
    colors.reserve(source.colors.size());
    for(;iter != source.colors.end();++iter)
        colors.push_back(Eigen::Vector3d((*iter)(0),(*iter)(1),(*iter)(2)));
}

Pointcloud::Extents Pointcloud::getExtents() const
{
    //TODO: Implement some sort of caching
    Extents res;
    for(size_t i=0;i<vertices.size();i++)
    {
	res.extend( vertices[i] );
    }
    return res; 
}

void Pointcloud::setSensorOrigin(const Transform& origin)
{
    this->sensor_origin = origin;
}

const Transform& Pointcloud::getSensorOrigin() const
{
    return sensor_origin;
}
