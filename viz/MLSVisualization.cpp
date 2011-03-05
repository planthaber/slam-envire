#include "MLSVisualization.hpp"
#include <osg/Group>
#include <osg/Geode>
#include <osg/Point>
#include <osg/Geometry>
#include <envire/Core.hpp>
#include <envire/maps/MultiLevelSurfaceGrid.hpp>
#include <osg/Drawable>
#include <osg/ShapeDrawable>
#include <osg/LineWidth>

MLSVisualization::MLSVisualization()
    : horizontalCellColor(osg::Vec4(0.1,0.5,0.9,1.0)), 
    verticalCellColor(osg::Vec4(0.8,0.9,0.5,1.0)), 
    uncertaintyColor(osg::Vec4(0.5,0.1,0.1,0.3)), 
    showUncertainty(true)
{
}

osg::Vec3 Vec3( const Eigen::Vector3d& v )
{
    return osg::Vec3( v.x(), v.y(), v.z() );
}

osg::Vec3 Vec3( const Eigen::Vector2d& v )
{
    return osg::Vec3( v.x(), v.y(), 0 );
}

class ExtentsRectangle : public osg::Geode
{
    osg::ref_ptr<osg::Geometry> geom;
    osg::ref_ptr<osg::Vec4Array> color;
    osg::ref_ptr<osg::Vec3Array> vertices;

public:
    ExtentsRectangle( const envire::GridBase::Extents& extents ) :
	geom( new osg::Geometry() ),
	color( new osg::Vec4Array() ), 
	vertices( new osg::Vec3Array() )
    {
	Eigen::Vector2d min = extents.min(), max = extents.max();
	vertices->push_back( osg::Vec3( min.x(), min.y(), 0 ));
	vertices->push_back( osg::Vec3( min.x(), max.y(), 0 ));
	vertices->push_back( osg::Vec3( max.x(), max.y(), 0 ));
	vertices->push_back( osg::Vec3( max.x(), min.y(), 0 ));

	geom->setVertexArray(vertices);
	osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays( osg::PrimitiveSet::LINE_LOOP, 0, vertices->size() );
	geom->addPrimitiveSet(drawArrays.get());

	color->push_back( osg::Vec4( 0.0f, 0.9f, 0.1f, 0.8f ) );
	geom->setColorArray(color.get());
	geom->setColorBinding( osg::Geometry::BIND_OVERALL );

	addDrawable(geom.get());    

	osg::StateSet* ss = getOrCreateStateSet();
	ss->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
	ss->setAttribute( new osg::LineWidth( 2.0 ) );
    }
};

osg::Group* MLSVisualization::getNodeForItem(envire::EnvironmentItem* item) const
{
    osg::ref_ptr<osg::Group> group = new osg::Group();
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    
    group->addChild(geode.get());
    
    updateNode(item, group);
    
    return group.release();
}

bool MLSVisualization::handlesItem(envire::EnvironmentItem* item) const
{
    return dynamic_cast<envire::MultiLevelSurfaceGrid *>(item);
}

void MLSVisualization::highlightNode(envire::EnvironmentItem* item, osg::Group* group) const
{
    // TODO handle highlight and unhighlight
}

void MLSVisualization::unHighlightNode(envire::EnvironmentItem* item, osg::Group* group) const
{
    // TODO handle highlight and unhighlight
}

void drawBox(osg::ref_ptr<osg::Vec3Array> vertices, osg::ref_ptr<osg::Vec3Array> normals, osg::ref_ptr<osg::Vec4Array> colors,  const osg::Vec3& position, const osg::Vec3& extents, const osg::Vec4& color )
{
    const double xp = position.x();
    const double yp = position.y();
    const double zp = position.z();

    const double xs = extents.x();
    const double ys = extents.y();
    const double zs = extents.z();

    vertices->push_back(osg::Vec3(xp-xs*0.5, yp-ys*0.5, zp+zs*0.5));
    vertices->push_back(osg::Vec3(xp+xs*0.5, yp-ys*0.5, zp+zs*0.5));
    vertices->push_back(osg::Vec3(xp+xs*0.5, yp+ys*0.5, zp+zs*0.5));
    vertices->push_back(osg::Vec3(xp-xs*0.5, yp+ys*0.5, zp+zs*0.5));
    for(size_t i=0;i<4;i++)
    {
	normals->push_back(osg::Vec3(0,0,1.0));
	colors->push_back(color);
    }

    if( zs > 0.0 )
    {
	vertices->push_back(osg::Vec3(xp-xs*0.5, yp-ys*0.5, zp+zs*0.5));
	vertices->push_back(osg::Vec3(xp+xs*0.5, yp-ys*0.5, zp+zs*0.5));
	vertices->push_back(osg::Vec3(xp+xs*0.5, yp-ys*0.5, zp-zs*0.5));
	vertices->push_back(osg::Vec3(xp-xs*0.5, yp-ys*0.5, zp-zs*0.5));
	for(size_t i=0;i<4;i++)
	{
	    normals->push_back(osg::Vec3(0,-1.0,0));
	    colors->push_back(color);
	}

	vertices->push_back(osg::Vec3(xp+xs*0.5, yp-ys*0.5, zp+zs*0.5));
	vertices->push_back(osg::Vec3(xp+xs*0.5, yp+ys*0.5, zp+zs*0.5));
	vertices->push_back(osg::Vec3(xp+xs*0.5, yp+ys*0.5, zp-zs*0.5));
	vertices->push_back(osg::Vec3(xp+xs*0.5, yp-ys*0.5, zp-zs*0.5));
	for(size_t i=0;i<4;i++)
	{
	    normals->push_back(osg::Vec3(1.0,0,0));
	    colors->push_back(color);
	}

	vertices->push_back(osg::Vec3(xp+xs*0.5, yp+ys*0.5, zp+zs*0.5));
	vertices->push_back(osg::Vec3(xp-xs*0.5, yp+ys*0.5, zp+zs*0.5));
	vertices->push_back(osg::Vec3(xp-xs*0.5, yp+ys*0.5, zp-zs*0.5));
	vertices->push_back(osg::Vec3(xp+xs*0.5, yp+ys*0.5, zp-zs*0.5));
	for(size_t i=0;i<4;i++)
	{
	    normals->push_back(osg::Vec3(0,1.0,0));
	    colors->push_back(color);
	}

	vertices->push_back(osg::Vec3(xp-xs*0.5, yp+ys*0.5, zp+zs*0.5));
	vertices->push_back(osg::Vec3(xp-xs*0.5, yp-ys*0.5, zp+zs*0.5));
	vertices->push_back(osg::Vec3(xp-xs*0.5, yp-ys*0.5, zp-zs*0.5));
	vertices->push_back(osg::Vec3(xp-xs*0.5, yp+ys*0.5, zp-zs*0.5));
	for(size_t i=0;i<4;i++)
	{
	    normals->push_back(osg::Vec3(-1.0,0,0));
	    colors->push_back(color);
	}

	vertices->push_back(osg::Vec3(xp-xs*0.5, yp-ys*0.5, zp-zs*0.5));
	vertices->push_back(osg::Vec3(xp+xs*0.5, yp-ys*0.5, zp-zs*0.5));
	vertices->push_back(osg::Vec3(xp+xs*0.5, yp+ys*0.5, zp-zs*0.5));
	vertices->push_back(osg::Vec3(xp-xs*0.5, yp+ys*0.5, zp-zs*0.5));
	for(size_t i=0;i<4;i++)
	{
	    normals->push_back(osg::Vec3(0,0,-1.0));
	    colors->push_back(color);
	}
    }
}

void MLSVisualization::updateNode(envire::EnvironmentItem* item, osg::Group* group) const
{
    osg::ref_ptr<osg::Geode> geode = group->getChild(0)->asGeode();
    //remove old drawables
    while(geode->removeDrawables(0));

    envire::MultiLevelSurfaceGrid *mls = dynamic_cast<envire::MultiLevelSurfaceGrid *>(item);
    assert(mls);

    // add extents
    group->removeChild( extents );
    extents = new ExtentsRectangle( mls->getExtents() );
    group->addChild( extents );
    
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    osg::ref_ptr<osg::Vec4Array> color = new osg::Vec4Array;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    
    const double xs = mls->getScaleX();
    const double ys = mls->getScaleY();

    osg::ref_ptr<osg::Vec3Array> var_vertices = new osg::Vec3Array;

    //std::cerr << "grid size: " << mls->getWidth() << " x " << mls->getHeight() << std::endl;
    int hor = 0;
    for(size_t x=0;x<mls->getWidth();x++)
    {
	for(size_t y=0;y<mls->getHeight();y++)
	{
	    for( envire::MultiLevelSurfaceGrid::const_iterator it = mls->beginCell_const( x, y ); it != mls->endCell_const(); it++ )
	    {
		const envire::MultiLevelSurfaceGrid::SurfacePatch &p(*it);
		double xp = (x+0.5) * xs;
		double yp = (y+0.5) * ys; 

		if( p.horizontal == true )
		{
		    drawBox( vertices, normals, color, osg::Vec3( xp, yp, p.mean ), osg::Vec3( xs, ys, 0.0 ), horizontalCellColor );
		    hor++;
		}
		else
		{
		    drawBox( vertices, normals, color, osg::Vec3( xp, yp, p.mean-p.height*.5 ), osg::Vec3( xs, ys, p.height ), verticalCellColor );
		}

		if( showUncertainty )
		{
		    var_vertices->push_back( osg::Vec3( xp, yp, p.mean - p.height * 0.5 + (p.height * 0.5 + p.stdev) ) );
		    var_vertices->push_back( osg::Vec3( xp, yp, p.mean - p.height * 0.5 - (p.height * 0.5 + p.stdev) ) );

		    /*
		    const double sizefactor = 0.2;
		    drawBox( vertices, normals, color, 
			    osg::Vec3( xp, yp, p.mean-p.height*.5 ), 
			    osg::Vec3( xs*sizefactor, ys*sizefactor, p.height+2.0*p.stdev ), 
			    uncertaintyColor );
			    */
		}
	    }
	}
    }
    //std::cerr << "vertices: " << vertices->size() << " hor: " << hor << std::endl;

    geom->setVertexArray(vertices);
    osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays( osg::PrimitiveSet::QUADS, 0, vertices->size() );
    geom->addPrimitiveSet(drawArrays.get());

    geom->setNormalArray(normals);
    geom->setNormalBinding( osg::Geometry::BIND_PER_VERTEX );

    geom->setColorArray(color.get());
    geom->setColorBinding( osg::Geometry::BIND_PER_VERTEX );

    geode->addDrawable(geom.get());    

    if( showUncertainty )
    {
	osg::ref_ptr<osg::Geometry> var_geom = new osg::Geometry;
	var_geom->setVertexArray( var_vertices );
	osg::ref_ptr<osg::DrawArrays> drawArrays = new osg::DrawArrays( osg::PrimitiveSet::LINES, 0, var_vertices->size() );
	var_geom->addPrimitiveSet(drawArrays.get());

	osg::ref_ptr<osg::Vec4Array> var_color = new osg::Vec4Array;
	var_color->push_back( osg::Vec4( 0.5, 0.1, 0.8, 1.0 ) );
	var_geom->setColorArray( var_color.get() );
	var_geom->setColorBinding( osg::Geometry::BIND_OVERALL );

	geode->addDrawable( var_geom.get() );
    }
}
