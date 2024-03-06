/**
 * @file MonitorableObject.cpp
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include <opmonlib/MonitorableObject.hpp>
#include <opmonlib/Utils.hpp>
#include <logging/Logging.hpp>

#include <google/protobuf/util/time_util.h>

#include <chrono>

using namespace dunedaq::opmonlib;

void MonitorableObject::register_child( std::string name, new_child_ptr p ) {

  std::unique_lock<std::mutex> lock(m_children_mutex);

  // check if the name is already present to ensure uniqueness
  auto it = m_children.find(name) ;
  if ( it != m_children.end() ) {
    // This not desired because names are suppposed to be unique
    // But if the pointer is expired, there is no harm in override it
    if ( it -> second.expired() ) {
      ers::warning(NonUniqueChildName(ERS_HERE, name));
    }
    else {
      throw NonUniqueChildName(ERS_HERE, name);
    }
  }
  
  m_children[name] = p;

  p -> m_opmon_name = name;
  p -> inherit_parent_properties( *this );

  TLOG() << "Child " << name << " registered to " << get_opmon_id() ;
}


void MonitorableObject::publish( google::protobuf::Message && m ) const noexcept {

  auto timestamp = google::protobuf::util::TimeUtil::GetCurrentTime();

  auto e = to_entry( m );

  if ( e.data().empty() ) {
    ers::warning( EntryWithNoData(ERS_HERE, e.measurement() ) );
    return ;
  }
  
  e.set_opmon_id( get_opmon_id() );
  *e.mutable_time() = timestamp;

  // this pointer is always garanteed to be filled, even if with a null Facility.
  // But the facility can fail
  try {
    m_facility->publish(std::move(e));
  } catch ( const OpMonPublishFailure & e ) {
    ers::error(e);
  }
 
}


opmon::MonitoringTreeInfo MonitorableObject::collect( opmon_level l) noexcept {

  auto start_time = std::chrono::high_resolution_clock::now();

  TLOG() << "Collecting data from " << get_opmon_id();
  
  opmon::MonitoringTreeInfo info;

  info.set_n_invalid_links(0);

  int n_metrics = 0;
  try {
    n_metrics = generate_opmon_data();
  } catch ( const ers::Issue & i ) {
    n_metrics = -1;
    auto cause_ptr = i.cause();
    while ( cause_ptr ) {
      --n_metrics;
      cause_ptr = cause_ptr->cause();
    }
    ers::error( ErrorWhileCollecting(ERS_HERE, i) );
  } catch (  const std::exception & e ) {
    n_metrics = -1;
    ers::error( ErrorWhileCollecting(ERS_HERE, e) );
  } catch (...) {
    n_metrics = -1;
    ers::error( ErrorWhileCollecting(ERS_HERE) );
  }
    
  if (n_metrics>0) {
    info.set_n_publishing_nodes(1);
    info.set_n_published_measurements( n_metrics );
  } else {
    info.set_n_errors( -n_metrics );
  }

  std::unique_lock<std::mutex> lock(m_children_mutex);

  info.set_n_registered_nodes( m_children.size() );

  unsigned int n_invalid_links = 0;
  
  for ( auto it = m_children.begin(); it != m_children.end(); ) {

    auto ptr = it->second.lock();
    
    if( ptr ) {
      auto child_info = ptr->collect(l);  // MR: can we make this an async? There is no point to wait all done here
      info.set_n_registered_nodes( info.n_registered_nodes() + child_info.n_registered_nodes() );
      info.set_n_publishing_nodes( info.n_publishing_nodes() + child_info.n_publishing_nodes() );
      info.set_n_invalid_links( info.n_invalid_links() + child_info.n_invalid_links() );
      info.set_n_published_measurements( info.n_published_measurements() + child_info.n_published_measurements() );
      info.set_n_errors( info.n_errors() + child_info.n_errors() );
    }

    // prune the dead links
    if ( it->second.expired() ) {
      it = m_children.erase(it);
      ++n_invalid_links;
    } else {
      ++it;
    }
  }

  info.set_n_invalid_links( info.n_invalid_links() + n_invalid_links );
  
  
  auto stop_time = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::microseconds>( stop_time - start_time );
  info.set_clockwall_elapsed_time_us( duration.count() );
  info.set_cpu_elapsed_time_us( duration.count() ); // MR: this is ok for now but needs changing if we use async collection
  
  return info;
}


void MonitorableObject::inherit_parent_properties( const MonitorableObject & parent ) {

  m_facility = parent.m_facility;
  m_parent_opmon_id = parent.get_opmon_id();
  
  std::unique_lock<std::mutex> lock(m_children_mutex);

  for ( const auto & [key,wp] : m_children ) {

    auto p = wp.lock();
    if ( p ) {
      p->inherit_parent_properties(*this);
    }
    
  }
  
}


