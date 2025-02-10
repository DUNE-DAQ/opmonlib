/**
 * @file MonitorableObject.hpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef OPMONLIB_INCLUDE_OPMONLIB_MONITORABLEOBJECT_HPP_
#define OPMONLIB_INCLUDE_OPMONLIB_MONITORABLEOBJECT_HPP_

#include "opmonlib/Utils.hpp"
#include "opmonlib/OpMonFacility.hpp"
#include <opmonlib/opmon/monitoring_tree.pb.h>

#include "confmodel/OpMonConf.hpp"
#include <logging/Logging.hpp> // NOTE: if ISSUES ARE DECLARED BEFORE include logging/Logging.hpp, TLOG_DEBUG<<issue wont work.

#include <google/protobuf/message.h>

#include <limits>
#include <type_traits>
#include <map>
#include <memory>
#include <string>
#include <mutex>

namespace dunedaq {

  ERS_DECLARE_ISSUE( opmonlib,
		     NonUniqueNodeName,
		     name << " already present in the node list of " << opmon_id,
		     ((std::string)name)((std::string)opmon_id) 
		     )
	  
  ERS_DECLARE_ISSUE( opmonlib,
		     EntryWithNoData,
		     "OpMonEntry of type " << type << " has no data",
		     ((std::string)type)
		   )

  ERS_DECLARE_ISSUE( opmonlib,
                     ErrorWhileCollecting,
                     "OpMon data collection failed in " << opmon_id,
                     ((std::string)opmon_id) )

}


namespace dunedaq::opmonlib {

  using OpMonLevel = std::invoke_result<decltype(&dunedaq::confmodel::OpMonConf::get_level),
					dunedaq::confmodel::OpMonConf>::type;
  
  enum class SystemOpMonLevel : OpMonLevel {
    kDisabled = std::numeric_limits<OpMonLevel>::min(),
    kAll      = std::numeric_limits<OpMonLevel>::max()
  };

  enum class EntryOpMonLevel : OpMonLevel {
    kTopPriority     = std::numeric_limits<OpMonLevel>::min(),
    kEventDriven     = std::numeric_limits<OpMonLevel>::max()/4,
    kDefault         = std::numeric_limits<OpMonLevel>::max()/2,
    kLowestPrioriry  = std::numeric_limits<OpMonLevel>::max()-1
  };

  template <class T>
  constexpr auto to_level( T v ) {
    return static_cast<typename std::underlying_type<T>::type>(v);
  }
  
  
class MonitorableObject
{
public:

  using NodePtr = std::weak_ptr<MonitorableObject>;
  using NewNodePtr = std::shared_ptr<MonitorableObject>;
  using ElementId = std::string;

  friend class OpMonManager;
  
  /**
   * copy and move constructors are deleted as they violate the linking chain
   */
  MonitorableObject( const MonitorableObject & ) = delete ;
  MonitorableObject & operator = ( const MonitorableObject &) = delete;
  MonitorableObject( MonitorableObject && ) = delete;
  MonitorableObject & operator = (MonitorableObject &&) = delete;    
  
  virtual ~MonitorableObject() = default;
  
  auto get_opmon_id() const noexcept { return m_parent_id + m_opmon_name; }

  auto get_opmon_level() const noexcept { return m_opmon_level.load(); }
  
  static bool publishable_metric( OpMonLevel entry, OpMonLevel system ) noexcept {
    return (entry < system);
  }
  
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
  void register_node( ElementId name, NewNodePtr ) ;

  /**
   * Convert the message into an OpMonEntry and then uses the pointer to the Facility to publish the entry.
   * This also timestamps the message with the time of the invocation.
   * It is possible to associate an element name to the published message.
   *    the element name is checked against the children to protect uniqueness. 
   * It is also possible to associate a custom origin in the form of a map<string,string>. 
   *    This is designed to add information which is independent from the software structure
   *    e.g.  channels or other hardware information.
   * Messages will have an associated OpmonLevel that is used to suppress the pubblication of metrics. 
   *    The level of the message is set by the OpMonManager.
   *    
   */
  void publish( google::protobuf::Message &&,
		CustomOrigin && co = {},
		OpMonLevel l = to_level(EntryOpMonLevel::kDefault) ) const noexcept ;

  /**
   * Hook for customisable pubblication. 
   * The function can throw, exception will be caught by the monitoring thread
   */
  virtual void generate_opmon_data() {return;}

  
private:

  /**
   * Instructs the object to pusblish regular interval metrics.
   * It also instruct the children to execute their collect methods.
   * 
   * \return It returns a protobuf schema object to monitor the tree
   */     
  opmon::MonitoringTreeInfo collect() noexcept ;
  
  /**
   * Hook to propagate the OpMonLevel at lower levels of the monitoring tree
   */
  void set_opmon_level( OpMonLevel ) noexcept; 

  /**
   * utilities for linking with parent and top levels
   */
  void inherit_parent_properties( const MonitorableObject & parent );   // funcion called on the children as well

   /**
   * Contructor to set initial strings
   */ 
  MonitorableObject( ElementId name, ElementId parent_id = "" )
    : m_parent_id()
    , m_opmon_name(name) {
    m_parent_id.set_session(parent_id);
  }
  
  std::map<ElementId, NodePtr> m_nodes ;
  std::mutex m_node_mutex;

  using facility_ptr_t = std::shared_ptr<opmonlib::OpMonFacility>;
  std::atomic<facility_ptr_t> m_facility{s_default_facility};
  dunedaq::opmon::OpMonId m_parent_id;
  std::atomic<OpMonLevel> m_opmon_level = to_level(SystemOpMonLevel::kAll);
  ElementId m_opmon_name;

  static facility_ptr_t s_default_facility;
  
  // info for monitoring the monitoring structure
  using const_metric_counter_t = std::invoke_result<decltype(&dunedaq::opmonlib::opmon::MonitoringTreeInfo::n_published_measurements),
						    dunedaq::opmonlib::opmon::MonitoringTreeInfo>::type;
  using metric_counter_t = std::remove_const<const_metric_counter_t>::type;
  mutable std::atomic<metric_counter_t> m_published_counter{0};
  mutable std::atomic<metric_counter_t> m_ignored_counter{0};
  mutable std::atomic<metric_counter_t> m_error_counter{0};

  using const_time_counter_t = std::invoke_result<decltype(&dunedaq::opmonlib::opmon::MonitoringTreeInfo::cpu_elapsed_time_us),
						  dunedaq::opmonlib::opmon::MonitoringTreeInfo>::type;
  using time_counter_t = std::remove_const<const_metric_counter_t>::type;
  mutable std::atomic<time_counter_t> m_cpu_us_counter{0};
};


  class OpMonLink : public MonitorableObject {
  public:
    using MonitorableObject::register_node;
  };
  
} // namespace dunedaq::opmonlib

#endif // OPMONLIB_INCLUDE_OPMONLIB_MONITORABLEOBJECT_HPP_
