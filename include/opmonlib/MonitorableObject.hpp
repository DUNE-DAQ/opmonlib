/**
 * @file MonitorableObject.hpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef OPMONLIB_INCLUDE_OPMONLIB_MONITORABLEOBJECT_HPP_
#define OPMONLIB_INCLUDE_OPMONLIB_MONITORABLEOBJECT_HPP_

#include "opmonlib/OpMonFacility.hpp"
#include <google/protobuf/message.h>
#include <opmonlib/info/MonitoringTreeInfo.pb.h>

#include <map>
#include <memory>
#include <string>
#include <mutex>

namespace dunedaq {

  ERS_DECLARE_ISSUE( opmonlib,
		     NonUniqueChildName,
		     name << " already present in the child list",
		     ((std::string)name)
		     )
	  
  ERS_DECLARE_ISSUE( opmonlib,
		     EntryWithNoData,
		     "OpMonEntry of type " << type << " has no data",
		     ((std::string)type)
		   )

  ERS_DECLARE_ISSUE( opmonlib,
                     ErrorWhileCollecting,
                     "OpMon data collection failed",
                     ERS_EMPTY )

}


namespace dunedaq::opmonlib {

class MonitorableObject
{
public:

  using child_ptr = std::weak_ptr<MonitorableObject>;
  using new_child_ptr = std::shared_ptr<MonitorableObject>;
  using opmon_level = uint32_t; // NOLINT(build/unsigned)     
  using opmon_id = std::string;

  friend class OpMonManager;
  
  /**
   * copy and move constructors are deleted as they violate the linking chain
   */
  MonitorableObject( const MonitorableObject & ) = delete ;
  MonitorableObject & operator = ( const MonitorableObject &) = delete;
  MonitorableObject( MonitorableObject && ) = delete;
  MonitorableObject & operator = (MonitorableObject &&) = delete;    
  
  virtual ~MonitorableObject() {;}
  
  opmon_id get_opmon_id() const noexcept {
    return m_parent_opmon_id.empty() ? m_opmon_name : m_parent_opmon_id + '.' + m_opmon_name; }

protected:

  /**
   * default constructors are ok as they set the links correctly
   * i.e. The service points to the null and the names are not set
   */
  MonitorableObject() = default;

  /**
    * Append a register object to the chain
    * The children will be modified using information from the this parent
    */
  void register_child( std::string name, new_child_ptr ) ;

  /**
   * Convert the message into an OpMonEntry and then uses the pointer to the Facility to publish the entry
   * This also timestamps the message with the time of the invocation
   */
  void publish( google::protobuf::Message && ) const noexcept ;

  /**
   * Hook for customisable pubblication. 
   * The function can throw, exception will be caught by the monitoring thread
   * 
   * \return The function is supposed to return the number of published measurements
   *         this is used for monitoring purposes. 
   *         It can return negative values to signal an error, without throwing 
   */
  virtual int generate_opmon_data() {return 0;}

private:

  /**
   * Instructs the object to pusblish regular interval metrics.
   * It also instruct the children to execute their collect methods.
   * 
   * \return It returns a protobuf schema object to monitor the tree
   */     
  opmon::MonitoringTreeInfo collect(opmon_level) noexcept ;
    
  /**
   * utilities for linking with parent and top levels
   */
  void inherit_parent_properties( const MonitorableObject & parent );   // funcion called on the children as well

  /**
   * Contructor to set initial strings
   */ 
  MonitorableObject( opmon_id name, opmon_id parent_id = "" )
    : m_parent_opmon_id(parent_id)
    , m_opmon_name(name) {;}
  
  std::map<std::string, child_ptr> m_children ;
  std::mutex m_children_mutex;

  std::shared_ptr<opmonlib::OpMonFacility> m_facility = makeOpMonFacility("null://");
  opmon_id m_parent_opmon_id;
  opmon_id m_opmon_name;
};

} // namespace dunedaq::opmonlib

#endif // OPMONLIB_INCLUDE_OPMONLIB_MONITORABLEOBJECT_HPP_
