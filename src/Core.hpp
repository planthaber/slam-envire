#ifndef __ENVIRE_H__
#define __ENVIRE_H__

#include <list>
#include <map>
#include <string>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <boost/intrusive_ptr.hpp>
#include <vector>
#include <stdexcept>

#include <envire/core/EventSource.hpp>
#include <envire/core/EventTypes.hpp>
#include <envire/core/Transform.hpp>
#include <envire/core/Holder.hpp>
#include <base/samples/rigid_body_state.h>

#define ENVIRONMENT_ITEM_DEF( _classname ) \
const std::string _classname::className = "envire::" #_classname; \
static envire::SerializationPlugin<_classname> _classname ## factory;

#define ENVIRONMENT_ITEM( _classname ) \
	public:\
	static const std::string className; \
	const std::string& getClassName() const { return className; } \
	void set( EnvironmentItem* other ) \
	{\
	    _classname* fn = dynamic_cast<_classname*>( other ); \
	    if( fn ) \
	    {\
		this->operator=( *fn );\
	    }\
	}\
	typedef boost::intrusive_ptr<_classname> Ptr; \
	_classname* clone() const \
	{ \
	    return new _classname( *this );\
	}\
	private:\

namespace envire
{
    class Layer;
    class CartesianMap;
    class FrameNode;
    class Operator;
    class Environment;
    class EnvironmentItem;
    class Serialization;
    class EventHandler;
    class Event;
    class SerializationFactory;

    /** Base class for alle items that are defined in the envire framework.
     * Mainly handles the unique_id feature and the pointer to the environment
     * object.
     *
     * The unique_id of an item is a string representation with an optional
     * integer part. Ids given on construction of an EnvironmentItem based
     * object are first prefixed by the environment prefix (which defaults to
     * '/'), then the provided string is used to form an unique identifier. If
     * this identifier is already in the environment, attaching it will throw.
     * There is however the option to add a trailing '/' to the id. In this
     * case, attaching the item in this case will not generate an exception,
     * but will make the item unique by appending a number to make the complete
     * id unique.
     *
     * [/<environment_prefix>]/<id>[/<numeric_id]
     *
     * Ownership of objects is managed as follows: Objects are owned by the user
     * as long as they are not attached to the environment.
     *
     * Ownership is passed to the Environment object, once the item is attached
     * to the environment in one of the following ways:
     *
     * - explicit: env->attachItem( item )
     * - implicit: using the item as parameter to a call like item2->addChild(
     *   item )
     *
     * When the ownership is passed, it is not allowed to free the memory of the
     * object, this is now done by the Environment on destruction.
     *
     * The ownership of the items can be passed back to the user by calling
     * env->detachItem( item )
     */
    class EnvironmentItem 
    {
    public:
	typedef boost::intrusive_ptr<EnvironmentItem> Ptr; 

    protected:
	friend class Environment;
	friend class Event;
	friend void intrusive_ptr_add_ref( EnvironmentItem* item );
	friend void intrusive_ptr_release( EnvironmentItem* item );

	long ref_count;

	/** each environment item must have a unique id.
	 */
	std::string unique_id;

	/** non-unique label which can be used for any purpose
	 */
	std::string label;
	
	/** store pointer to environment to allow convenience methods
	 * referencing the environment object the item is attached to.
	 */
	Environment* env;

    public:
	static const std::string className;
	
	explicit EnvironmentItem(std::string const& id);

	/** overide copy constructor, to allow copying, but remove environment
	 * information for the copied item */
	EnvironmentItem(const EnvironmentItem& item);
	EnvironmentItem& operator=(const EnvironmentItem& other);

	/** Creates a clone of this item. 
	 */
        virtual EnvironmentItem* clone() const { 
	    throw std::runtime_error("clone() not implemented. Did you forget to use the ENVIRONMENT_ITEM macro?."); }

	/** virtual assignemt of other value to this
	 */
	virtual void set( EnvironmentItem* other ) { 
	    throw std::runtime_error("set() not implemented. Did you forget to use the ENVIRONMENT_ITEM macro?."); }

	/** will attach the newly created object to the given Environment.
	 */ 
	explicit EnvironmentItem(Environment* env);	

	virtual ~EnvironmentItem();

	virtual void serialize(Serialization &so);
        
        virtual void unserialize(Serialization &so);

	virtual const std::string& getClassName() const {return className;};

	/** @return the environment this object is associated with 
	 */
	Environment* getEnvironment() const;

	/** @return true if attached to an environment
	 */	
	bool isAttached() const;

	/** Sets all or part of the uniqe ID for this item.
         *
         * This is made unique by the environment when this item is attached to
         * an environment. This method will raise logic_error if used after the
         * item has been attached.
	 */
	void setUniqueId(std::string const& id);
        
	/** @return the unique id of this environmentitem
	 */
	std::string getUniqueId() const;
        
        /** @return the environment prefix, which is part of the unique id
         */
        std::string getUniqueIdPrefix() const;
        
        /** @return the suffix (last part after the /) of the unique id
         */
        std::string getUniqueIdSuffix() const;

	/** @return the suffix of the unique id and perform a conversion to
	 * integer type
	 *
	 * will throw if suffix is not actually numerical, which happens, when
	 * the original unique id given had a trailing slash.
         */
        long getUniqueIdNumericalSuffix() const;

	/** marks this item as modified
	 */
	void itemModified();

	/** will detach the item from the current environment
	 */
	EnvironmentItem::Ptr detach();

	/** get the non-unique label attached to this item
	 */
	std::string getLabel() const { return label; }

	/** set the non-unique label attached to this item
	 */
	void setLabel( const std::string& label ) { this->label = label; }
    };

    void intrusive_ptr_add_ref( EnvironmentItem* item );
    void intrusive_ptr_release( EnvironmentItem* item );
    
    template<class T> EnvironmentItem* createItem(Serialization &so) 
    {
	T* o = new T();
        o->unserialize(so);
	return o;
    }

    class SerializationFactory 
    {
    public:
	/** Factory typedef, which needs to be implemented by all EnvironmentItems
	 * that are serialized.
	 */
	typedef EnvironmentItem* (*Factory)(Serialization &);

	/** Stores the mapping for all classes that can be serialized, and a function
	 * pointer to the Factory method of that class.
	 */
	static std::map<std::string, Factory>& getMap();

	/** create and object for the given class. Will throw if no such class is registered.
	 */
	static EnvironmentItem* createObject( const std::string& className, Serialization& so );

	/** register a class with the factory
	 */
	static void addClass( const std::string& className, Factory f );
    };


    /** An object of this class represents a node in the FrameTree. The
     * FrameTree has one root node (call to env->getRootNode()), which
     * represents the global frame. Each child defines a new frame of reference,
     * that is connected to its parent frame through the transformation object.
     *
     * The transformation C^P_C associated with this object will transform from 
     * this FrameNodes Frame into the parent Frame.
     *
     * A FrameNode holds a TransformWithUncertainty object representing the
     * transformation from child to parent node. This transformation may have
     * uncertainty associated with it, but doesn't have to. In the latter case,
     * a fast path for calculation of transformation chains can be used.
     */
    class FrameNode : public EnvironmentItem
    {
	ENVIRONMENT_ITEM( FrameNode )

    public:
	typedef Transform TransformType;

    public:
	/** class needs to be 16byte aligned for Eigen vectorisation */
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

        /** default constructor */
        FrameNode();
        explicit FrameNode(const TransformType &t);
        explicit FrameNode(const TransformWithUncertainty &t);

	virtual void serialize(Serialization &so);
        virtual void unserialize(Serialization &so);

        /** Returns true if this frame is the root frame (i.e. has no parent) */
        bool isRoot() const;

        /** Returns the frame that is parent of this one, or raises
         * @throw std::runtime_error if it is a root frame
         */
        const FrameNode* getParent() const;

        /** Returns the frame that is parent of this one, or raises
         */
        FrameNode* getParent();

        /** Returns the frame that is the root of the frame tree in which this
         * frame itself is stored
         */
        const FrameNode* getRoot() const;

        /** Returns the frame that is the root of the frame tree in which this
         * frame is stored
         */
        FrameNode* getRoot();

	/** Will add the @param child to the current list of children, if this
	 * item is attached.
	 */
	void addChild( FrameNode *child );

        /** Returns the transformation from this FrameNode to the parent framenode
         */
	TransformType const& getTransform() const;

        /** Updates the transformation between that node and its parent.
         * Relevant operators will be notified of that change, and all data that
         * has been generated based on that information will be marked as dirty
         */
        void setTransform(TransformType const& transform);

	/** @return the transformation from this FrameNode to the parent
	 * FrameNode with attached uncertainty.
	 */
	TransformWithUncertainty const& getTransformWithUncertainty() const;

	/** overloaded setTransform() which also sets the uncertainty associated
	 * with a transformation.
	 */
	void setTransform(TransformWithUncertainty const& transform);

        /** 
	 * @return the transformation from this frame to
         * the frame represented by @a to. This always defines an unique
         * transformation, as the frames are sorted in a tree.
	 * This is a convenince access to Environment::relativeTransform()
         */
	TransformType relativeTransform( const FrameNode* to ) const;

	/** 
	 * @return a list of maps attached to this framenode.
	 */
	std::list<CartesianMap*> getMaps(); 

	/** 
	 * @return the children of this framenode
	 */
	std::list<FrameNode*> getChildren();

    protected:
	TransformWithUncertainty frame;
    };

    /** The layer is the base object that holds map data. It can be a cartesian
     * map (CartesianMap) or a non-cartesian one (AttributeList, TopologicalMap)
     */
    class Layer : public EnvironmentItem
    {
    protected:
        /** @todo explain immutability for layer */
        bool immutable;

        /** @todo explain dirty for a layer */
        bool dirty; 

        typedef std::map<std::string, HolderBase*> DataMap;

	/** associating key values with metadata stored in holder objects */ 
	DataMap data_map;

    public:
	static const std::string className;

	/** @brief custom copy constructor required because of metadata handling.
	 */
	Layer(const Layer& other);

	/** @brief custom assignment operator requried because of metadata.
	 */
	Layer& operator=(const Layer& other);

	explicit Layer(std::string const& id);
	virtual ~Layer();

	void serialize(Serialization& so);
        void unserialize(Serialization& so);

	virtual const std::string& getClassName() const {return className;};

        /** @return True if this layer cannot be changed by any means */
        bool isImmutable() const;

        /** Marks this frame as being immutable. This cannot be changed
         * afterwards, as some operators will depend on it.
         */
        void setImmutable();

        /** Unsets the dirty flag on this layer
         * @see isDirty
         */
        void resetDirty();

        /** Marks this layer as dirty
         * @see isDirty
         */
        void setDirty();

        /** In case this layer is auto-generated, this flag is true when the
         * layer sources have been updated but this layer has not yet been.
         */
        bool isDirty() const;

        /** Detach this layer from the operator that generates it, and returns
         * true if this operation was a success (not all operators support
         * this). After this method returned true, it is guaranteed that
         * isGenerated() returns false
         */
        bool detachFromOperator();

        /** True if this layer has been generated by an operator */
        bool isGenerated() const;
        
        /** Returns the operator that generated this layer, or raises
         * std::runtime_error if it is not a generated map
         */
        Operator* getGenerator() const;

	/** Recomputes this layer by applying the operator that has already
	 * generated this map. The actual operation will only be called if the
	 * dirty flag is set, so it is optimal to call it whenever an updated
	 * map is needed. After this call, it is guaranteed that isDirty()
	 * returns false.
         */
        void updateFromOperator();
	
	/** Layers can have hierarchical relationships. This function will add
	 * a child layer to this object.
	 * @param child - the child layer to add
	 */
	void addChild(Layer* child);

	/** @return the parent of this layer or NULL if the layer has no parent
	 */
	std::list<Layer*> getParents();

	/**
         * @return for a given path, it will return a suggestion for a filename 
         * to use when making this layer persistant
	 */
	const std::string getMapFileName( const std::string& path, const std::string& className ) const;

        /**
         * Like getMapFileName(), but allows to override the class name (i.e. not
         * use the one from the map's class directly). This is meant to be used
         * for backward compatibility, when map class names change.
         */
	const std::string getMapFileName( const std::string& className ) const;
        
        /** @return a suggestion for a filename to use when making this layer persistant
         */
        const std::string getMapFileName() const;

	/** will return true if an entry for metadata for the given key exists
	 */
	bool hasData(const std::string& type) const;

        /** Will return true if a metadata exists for the given key, and this
         * metadata is of the specified type
         */
        template<typename T>
        bool hasData(const std::string& type) const
        {
            DataMap::const_iterator it = data_map.find(type);
            if (it == data_map.end())
                return false;
            else return it->second->isOfType<T>();
        }

	/** For a given key, return the metadata associated with it. If the data
	 * does not exist, it will be created.
	 * Will throw a runtime error if the datatypes don't match.
	 */
	template <typename T>
	    T& getData(const std::string& type)
	{
	    if( !hasData( type ) )
	    {
		data_map[type] = new Holder<T>;
	    }

	    /*
	    if( typeid(*data_map[type]) != typeid(Holder<T>) )
	    {
		std::cerr 
		    << "type mismatch. type should be " 
		    << typeid(data_map[type]).name() 
		    << " but is " 
		    << typeid(Holder<T>).name()
		    << std::endl;
		throw std::runtime_error("data type mismatch.");
	    }
	    */

	    return data_map[type]->get<T>();
	};
	
	/** 
	* For a given key, return the metadata associated with it. 
	* If the data does not exist, a std::out_of_range is thrown.
	*/
	template <typename T>
	const T& getData(const std::string& type) const
	{
	    std::map <std::string, envire::HolderBase* >::const_iterator it = data_map.find(type);
	    if(it == data_map.end())
		throw std::runtime_error("No metadata with name " + type + " available ");
	    
	    return it->second->get<T>();
	    /*
	    if( typeid(*data_map[type]) != typeid(Holder<T>) )
	    {
		std::cerr 
		    << "type mismatch. type should be " 
		    << typeid(data_map[type]).name() 
		    << " but is " 
		    << typeid(Holder<T>).name()
		    << std::endl;
		throw std::runtime_error("data type mismatch.");
	    }
	    */
	};

	/** @brief remove metadata with the specified identifier
	 */
	void removeData(const std::string& type);

	/** @brief remove all metadata associated with this object
	 */
	void removeData();
    };


    /** This is a special type of layer that describes a map in a cartesian
     * space. These maps have the special feature that they are managed in a
     * frame tree: each map is associated to a given frame (FrameNode). The
     * FrameNode objects themselves forming kinematic chains.
     */
    class CartesianMap : public Layer
    {
    public:
	typedef boost::intrusive_ptr<CartesianMap> Ptr; 

    public:
	static const std::string className;

	explicit CartesianMap(std::string const& id);

	virtual const std::string& getClassName() const {return className;};

        /** Sets the frame node on which this map is attached */
        void setFrameNode(FrameNode* frame);

        /** Returns the frame node on which this map is attached */
        FrameNode* getFrameNode();

        /** Returns the frame node on which this map is attached */
        const FrameNode* getFrameNode() const;

	/** @return the dimension of the cartesian space (2 or 3) */
	virtual int getDimension() const = 0;

        virtual CartesianMap* clone() const { throw std::runtime_error("clone() not implemented. Did you forget to use the ENVIRONMENT_ITEM macro?."); }

        /** Clones this map and the associated frame tree inside the target
         * environment
         */
        void cloneTo(Environment& env) const;
    };

    template <int _DIMENSION>
	class Map : public CartesianMap
    {
	static const int DIMENSION = _DIMENSION;

    public:

        // Defined later as it requires Environment
        Map();

        explicit Map(std::string const& id)
            : CartesianMap(id) {}

	typedef Eigen::AlignedBox<double, DIMENSION> Extents;

	int getDimension() const { return DIMENSION; }
	virtual Extents getExtents() const = 0;

        /** @overload
         *
         * It uses the root frame as the point's frame
         */
        Eigen::Vector3d toMap(Eigen::Vector3d const& point) const
        {
            return toMap(point, *getFrameNode()->getRoot());
        }

        /** Transforms a point from an arbitrary frame to the map's own frame
         *
         * This is valid only for maps of dimension less than 3
         */
        Eigen::Vector3d toMap(Eigen::Vector3d const& point, FrameNode const& frame) const
        {
            return frame.relativeTransform(getFrameNode()) * point;
        }

        /** @overload
         *
         * It uses the root frame as the target frame
         */
        Eigen::Vector3d fromMap(Eigen::Vector3d const& point) const
        {
            return toMap(point, *getFrameNode()->getRoot());
        }

        /** Transforms a point from the map's own frame to an arbitrary frame
         *
         * This is valid only for maps of dimension less than 3
         */
        Eigen::Vector3d fromMap(Eigen::Vector3d const& point, FrameNode const& frame) const
        {
            return getFrameNode()->relativeTransform(&frame) * point;
        }
    };

    /** An operator generates a set of output maps based on a set of input maps.
     * Operators are also managed by the Environment and can represent
     * operational releationsships between layers. Through the features of the
     * layer, which holds information about the update state (e.g. dirty), the
     * operators can be executed automatically on data that requires updates. 
     *
     * The links between operators and layers form a graph, which can represent
     * convenient operation chains. 
     *
     * E.g. raw_data -> [convert to map] -> map -> [merge into global map]
     * with map and [operator]
     */
    class Operator : public EnvironmentItem
    {
    public:
	typedef boost::intrusive_ptr<Operator> Ptr; 

	static const std::string className;

        /** Constructs a new operator
         *
         * @arg inputArity if nonzero, this is the number of inputs that this
         *                 operator requires
         * @arg outputArity if nonzero, this is the number of outputs that this
         *                  operator requires
         */
	explicit Operator(std::string const& id, int inputArity = 0, int outputArity = 0);

        /** Constructs a new operator
         *
         * @arg inputArity if nonzero, this is the number of inputs that this
         *                 operator requires
         * @arg outputArity if nonzero, this is the number of outputs that this
         *                  operator requires
         */
	explicit Operator(int inputArity = 0, int outputArity = 0);

        /** Update the output layer(s) according to the defined operation.
         */
        virtual bool updateAll(){return false;};

        /** Adds a new input to this operator. The operator may not support
         * this, in which case it will return false
         */
        virtual bool addInput(Layer* layer);

        /** Removes all existing inputs to this operator, and replaces them by
         * \c layer
         *
         * This is usually used for operators that accept only one input
         */
        bool setInput(Layer* layer);

        /** Removes an input from this operator. 
	 */ 
        virtual void removeInput(Layer* layer);

	/** Removes all inputs connected to this operator
	 */
	void removeInputs();

         /** Adds a new input to this operator. The operator may not support
         * this, in which case it will return false
         */
        virtual bool addOutput(Layer* layer);

        /** Removes all existing inputs to this operator, and replaces them by
         * \c layer
         *
         * This is usually used for operators that accept only one output
         */
        bool setOutput(Layer* layer);

        /** Removes an output from this operator.          
	 */
        virtual void removeOutput(Layer* layer);

	/** Removes all outputs connected to this operator
	 */
	void removeOutputs();

        /** Returns the only layer that is an output of type T for this operator
         *
         * LayerT must be a pointer-to-map type:
         * 
         * <code>
         * Grid<double>* slopes = mls_slope_operator->getOutput< Grid<double>* >();
         * </code>
         *
         * If multiple matching layers are found, an exception is raised
         */
        template<typename LayerT>
        LayerT getOutput();

        /** Returns the only layer that is an input of type LayerT for this
         * operator
         *
         * LayerT must be a pointer-to-map type:
         * 
         * <code>
         * MLSGrid* mls = mls_slope_operator->getInput< MLSGrid* >();
         * </code>
         *
         * If multiple matching layers are found, an exception is raised
         */
        template<typename LayerT>
        LayerT getInput();

    private:
        int inputArity;
        int outputArity;
    };

    /** The environment class manages EnvironmentItem objects and has ownership
     * of these.  all dependencies between the objects are handled in the
     * environment class, and convenience methods of the individual objects are
     * available to simplify usage.
     */
    class Environment 
    {
	friend class FileSerialization;
	friend class GraphViz;

	/** we track the last id given to an item, for assigning new id's.
	 */
        long last_id;
    public:
	static const std::string ITEM_NOT_ATTACHED;

    protected:
	typedef std::map<std::string, EnvironmentItem::Ptr > itemListType;
	typedef std::map<FrameNode*, FrameNode*> frameNodeTreeType;
	typedef std::multimap<Layer*, Layer*> layerTreeType;
	typedef std::multimap<Operator*, Layer*> operatorGraphType;
	typedef std::map<CartesianMap*, FrameNode*> cartesianMapGraphType;
	
	itemListType items;
	frameNodeTreeType frameNodeTree;
	layerTreeType layerTree;
	operatorGraphType operatorGraphInput;
	operatorGraphType operatorGraphOutput;
	cartesianMapGraphType cartesianMapGraph;
	
	FrameNode* rootNode;
        std::string envPrefix;

	EventSource eventHandlers;
	void publishChilds(EventHandler* handler, FrameNode *parent);
	void detachChilds(FrameNode *parent, EventHandler* handler);

    public:
        Environment();
	virtual ~Environment();
        
	/** attaches an EnvironmentItem and puts it under the control of the Environment.
	 * Object ownership is passed to the environment in this way.
	 */
	void attachItem(EnvironmentItem* item);

        /** Attaches a cartesian map to this environment. If the map does not
         * yet have a frame node, and none is given in this call, it is
         * automatically attached to the root node
         */
        void attachItem(CartesianMap* item, FrameNode* node = 0);

	/** detaches an object from the environment. After this, the object is no longer owned by
	 * by the environment. All links to this object from other objects in the environment are
	 * removed.
	 *
	 * If the deep param is left to false, this may lead to orphans, since
	 * a FrameNode may end up without a parent. Setting deep to true will
	 * remove all child nodes, as well as associated maps.
	 */
	EnvironmentItem::Ptr detachItem(EnvironmentItem* item, bool deep = false );
	
	/**
	* This method will be called by any EnvironmentItem, which was
	* modified. Calling this method will invoke all listeners 
	* attached to the environment and call their itemModified
	* method.
	**/
	void itemModified(EnvironmentItem* item);
	
	EnvironmentItem::Ptr getItem( const std::string& uniqueId ) const
	{
            itemListType::const_iterator it = items.find(uniqueId);
            if (it == items.end())
                return 0;
            else
                return it->second;
        }

        /** Returns the only item of the given type.
         *
         * This is a convenience method to search for an item (usually, a map)
         * in environments where it is known to be the only item of that type
         *
         * Throws std::runtime_error if there is not exactly one match (i.e.
         * either more than one or none)
         */
	template <class T>
	boost::intrusive_ptr<T> getItem() const
	{
            boost::intrusive_ptr<T> result;
            for (itemListType::const_iterator it = items.begin(); it != items.end(); ++it)
            {
                boost::intrusive_ptr<T> map = boost::dynamic_pointer_cast<T>(it->second);
                if (map)
                {
                    if (result)
                        throw std::runtime_error("multiple maps in this environment are of the specified type");
                    else
                        result = map;
                }
            }
            if (!result)
                throw std::runtime_error("no maps in this environment are of the specified type");
            return result;
        }

	template <class T>
	boost::intrusive_ptr<T> getItem( const std::string& uniqueId ) const
	{
            itemListType::const_iterator it = items.find(uniqueId);
            if (it == items.end())
                return 0;
            else
                return boost::dynamic_pointer_cast<T>(it->second);
        }

	void addChild(FrameNode* parent, FrameNode* child);
	void addChild(Layer* parent, Layer* child);

	void removeChild(FrameNode* parent, FrameNode* child);
	void removeChild(Layer* parent, Layer* child);

	FrameNode* getParent(FrameNode* node);
	std::list<Layer*> getParents(Layer* layer);

	FrameNode* getRootNode();
	std::list<FrameNode*> getChildren(FrameNode* parent);
	std::list<Layer*> getChildren(Layer* parent);
	std::list<const Layer*> getChildren(const Layer* parent) const;

	void setFrameNode(CartesianMap* map, FrameNode* node);
	void detachFrameNode(CartesianMap* map, FrameNode* node);
	
	FrameNode* getFrameNode(CartesianMap* map);
	std::list<CartesianMap*> getMaps(FrameNode* node);
	
	bool addInput(Operator* op, Layer* input);
	bool addOutput(Operator* op, Layer* output);

	bool removeInput(Operator* op, Layer* input);
	bool removeOutput(Operator* op, Layer* output);

	bool removeInputs(Operator* op);
	bool removeOutputs(Operator* op);

	std::list<Layer*> getInputs(Operator* op);
        
        /** Returns the only layer that is an input of type LayerT for the given
         * operator
         *
         * LayerT must be a pointer-to-map type:
         * 
         * <code>
         * MLSGrid* mls = env->getInput< MLSGrid* >(mls_slope_operator);
         * </code>
         *
         * If multiple matching layers are found, an exception is raised
         */
        template<typename LayerT>
        LayerT getInput(Operator* op)
        {
            std::list<Layer*> inputs = getInputs(op);
            LayerT result = 0;
            for (std::list<Layer*>::iterator it = inputs.begin(); it != inputs.end(); ++it)
            {
                LayerT layer = dynamic_cast<LayerT>(*it);
                if (layer)
                {
                    if (result)
                        throw std::runtime_error("more than one input layer with the required type found");
                    else
                        result = layer;
                }
            }
            if (!result)
                throw std::runtime_error("cannot find an input layer with the required type");
            return result;
        }

	std::list<Layer*> getOutputs(Operator* op);

        /** Returns the only layer that is an output of type T for the given
         * operator
         *
         * LayerT must be a pointer-to-map type:
         * 
         * <code>
         * Grid<double>* slopes = env->getOutput< Grid<double>* >(mls_slope_operator);
         * </code>
         *
         * If multiple matching layers are found, an exception is raised
         */
        template<typename LayerT>
        LayerT getOutput(Operator* op)
        {
            std::list<Layer*> outputs = getOutputs(op);
            LayerT result = 0;
            for (std::list<Layer*>::iterator it = outputs.begin(); it != outputs.end(); ++it)
            {
                LayerT layer = dynamic_cast<LayerT>(*it);
                if (layer)
                {
                    if (result)
                        throw std::runtime_error("more than one output layer with the required type found");
                    else
                        result = layer;
                }
            }
            if (!result)
                throw std::runtime_error("cannot find an output layer with the required type");
            return result;
        }

        /** Returns the operator that has \c output in its output, or NULL if
         * none exist.
         *
         * Note that a layer can be output of a single operator only
         */
	Operator* getGenerator(Layer* output);
        
        /** Returns the operators that has \c input in its input.
         */
        std::list<Operator*> getGenerators(Layer* input);

        /** Returns the layers that are generated from \c input
         *
         * In practice, it returns the output layers of all the operators for
         * which \c input is listed as input
         */
        std::list<Layer*> getLayersGeneratedFrom(Layer* input);

        /** Returns the layers of type T that are generated from \c input
         *
         * T must be a pointer on a map:
         *
         * <code>
         * env->getGeneratedFrom<Grid<double>*>(mls);
         * </code>
         */
        template<typename T>
        std::list<T> getGeneratedFrom(Layer* input)
        {
            std::list<Layer*> all = getLayersGeneratedFrom(input);
            std::list<T> result;
            for (std::list<Layer*>::const_iterator it = all.begin();
                    it != all.end(); ++it)
            {
                T as_T = dynamic_cast<T>(*it);
                if (as_T) result.push_back(as_T);
            }
            return result;
        }

	void updateOperators();

        /** Serializes this environment to the given directory */
        void serialize(std::string const& path);

        /** Loads the environment from the given directory and returns it */
        static Environment* unserialize(std::string const& path);

	/**
	 * Adds an eventHandler that gets called whenever there
	 * are modifications to the evironment.
	 * On registration, all content of the environment is put to the
	 * handler as if it was being generated.
	 */
	void addEventHandler(EventHandler *handler);

	/**
	 * Remove the given eventHandler from the environment.
	 * The handler will receive events as if the Environment was destroyed.
	 */
	void removeEventHandler(EventHandler *handler);

	/**
	 * will pass the @param event to the registered event handlers
	 */
	void handle( const Event& event );

	/**
	 * returns all items of a particular type
	 */
	template <class T>
	    std::vector<T*> getItems()
	{
	    std::vector<T*> result;
	    for(itemListType::iterator it=items.begin();it != items.end(); ++it )
	    {
		// TODO this is not very efficient...
		T* item = dynamic_cast<T*>( it->second.get() );
		if( item )
		    result.push_back( item );
	    }
	    return result;
	}

        //convenience function to create EnvironmentItems which are automatically attached
        //to the environment 
        template<class T>
        T* create()
        {
            T* p = new T;
            attachItem(p);
            return p;
        }

        /** 
	 * Returns the transformation from the frame represented by @a from to
         * the frame represented by @a to. This always defines an unique
         * transformation, as the frames are sorted in a tree.
	 *
	 * relativeTransform( child, child->getParent() ) is equivalent to
	 * child->getTransform().
         */
	FrameNode::TransformType relativeTransform(const FrameNode* from, const FrameNode* to);

        /** 
	 * @overload
         *
         * Returns the relative transformation between the frames of two
         * cartesian maps
         */
	FrameNode::TransformType relativeTransform(const CartesianMap* from, const CartesianMap* to);

	/** @return a new transform object, that specifies the transformation
	 * from the @param from frame to the @param to frame, and will take care of
	 * uncertainty (linearised) on the way.
	 */
	TransformWithUncertainty relativeTransformWithUncertainty(const FrameNode* from, const FrameNode* to);

        /** 
	 * @overload
         *
         * Returns the relative transformation, including uncertainty, between
         * the frames of two cartesian maps, 
         */
	TransformWithUncertainty relativeTransformWithUncertainty(const CartesianMap* from, const CartesianMap* to);
        
        /** Sets the prefix for ID generation for this environment
         *
         * The prefix is normalized to start and end with the '/' separation
         * marker. The prefix is used as a sort of namespace, so that
	 * ids can be kept unique between different environments.
         *
         * The default prefix is /
         */
        void setEnvironmentPrefix(std::string envPrefix);
        
        /** Returns the prefix for ID generation on this environment
         *
         * The default prefix is /
         */
        std::string getEnvironmentPrefix() const { return envPrefix; }

        /** Apply a set of serialized modifications to this environment
         */
        void applyEvents(std::vector<BinaryEvent> const& events);
    };
   
    template<typename LayerT>
    LayerT Operator::getInput()
    { return getEnvironment()->getInput<LayerT>(this); }

    template<typename LayerT>
    LayerT Operator::getOutput()
    { return getEnvironment()->getOutput<LayerT>(this); }

    /** This is defined here for backward compatibility reasons. The default
     * constructor for Map should be removed in the long run, as we want to make
     * sure that all map types define a constructor that allows to set the map
     * ID
     */
    template<int _DIM>
    Map<_DIM>::Map()
        : CartesianMap(Environment::ITEM_NOT_ATTACHED) {}
}

#endif
