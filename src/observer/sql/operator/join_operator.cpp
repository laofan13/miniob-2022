#include "common/log/log.h"
#include "sql/operator/join_operator.h"
#include "storage/record/record.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/common/field.h"


RC JoinOperator::open()
{
  if(nullptr == left_ || nullptr == right_) {
    LOG_WARN("join operator must has 2 child");
    return RC::INTERNAL;
  }

  RC rc = left_->open();
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  rc = right_->open();
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  return RC::SUCCESS;
}

RC JoinOperator::next()
{
 
  return RC::RECORD_EOF;
}

RC JoinOperator::close()
{
  RC rc = RC::SUCCESS;
  return rc;
}

Tuple * JoinOperator::current_tuple()
{
  return nullptr;
}


bool JoinOperator::do_predicate(Tuple *tuple)
{
  
  return true;
}