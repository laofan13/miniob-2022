#include "common/log/log.h"
#include "sql/operator/join_operator.h"
#include "storage/record/record.h"
#include "sql/stmt/filter_stmt.h"
#include "sql/stmt/join_stmt.h"
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

  while (RC::SUCCESS == (rc = left_->next())) {
    Tuple *tuple = left_->current_tuple();
    if (nullptr == tuple) {
      LOG_WARN("failed to get current record: %s", strrc(rc));
      return rc;
    }

    left_table_tuples.push_back(tuple);
  }

  while (RC::SUCCESS == (rc = right_->next())) {
    Tuple *tuple = right_->current_tuple();
    if (nullptr == tuple) {
      LOG_WARN("failed to get current record: %s", strrc(rc));
      return rc;
    }

    right_table_tuples.push_back(tuple);
  }

  total_num_ = left_table_tuples.size()*right_table_tuples.size();

  return RC::SUCCESS;
}

RC JoinOperator::next()
{
  CompositeTuple *composite_tuple = new CompositeTuple();
  while(current_index_ < total_num_) {
    composite_tuple->clear();
    int left_index = current_index_ / right_table_tuples.size();
    int right_index = current_index_ % right_table_tuples.size();

    auto left_tuple = left_table_tuples[left_index];
    auto tight_tuple = right_table_tuples[right_index];
    composite_tuple->add_tuple(left_tuple);
    composite_tuple->add_tuple(tight_tuple);
    
    if(!do_predicate(composite_tuple)) {
      current_index_++;
      continue;
    }
    current_index_++;
    composite_table_tuples_.push_back(composite_tuple);
    tuple_ = composite_tuple;
    return RC::SUCCESS;
  }
  delete composite_tuple;
  return RC::RECORD_EOF;
}

RC JoinOperator::close()
{
  RC rc = RC::SUCCESS;
  if ((rc = left_->close()) != RC::SUCCESS) {
    return rc;
  }
  if ((rc = right_->close()) != RC::SUCCESS) {
    return rc;
  }
  return rc;
}

Tuple * JoinOperator::current_tuple()
{
  return tuple_;
}


bool JoinOperator::do_predicate(Tuple *tuple)
{
  auto filter_stmt = join_unit_->filter_stmt();
  if (filter_stmt == nullptr || filter_stmt->filter_units().empty()) {
    return true;
  }

  for (const FilterUnit *filter_unit : filter_stmt->filter_units()) {
    Expression *left_expr = filter_unit->left();
    Expression *right_expr = filter_unit->right();
    CompOp comp = filter_unit->comp();
    TupleCell left_cell;
    TupleCell right_cell;

    if(left_expr->get_value(*tuple, left_cell) != RC::SUCCESS) {
      continue;
    }
    if(right_expr->get_value(*tuple, right_cell) != RC::SUCCESS) {
      continue;
    }

    // 有空值
    if(left_cell.attr_type() == NULLS || right_cell.attr_type() == NULLS) {
      if(left_cell.null_compare(right_cell,comp)) {
        continue;
      }
      return false;
    }

    int compare = left_cell.compare(right_cell);
    bool filter_result = false;
    switch (comp) {
    case EQUAL_TO: {
      filter_result = (0 == compare); 
    } break;
    case LESS_EQUAL: {
      filter_result = (compare <= 0); 
    } break;
    case NOT_EQUAL: {
      filter_result = (compare != 0);
    } break;
    case LESS_THAN: {
      filter_result = (compare < 0);
    } break;
    case GREAT_EQUAL: {
      filter_result = (compare >= 0);
    } break;
    case GREAT_THAN: {
      filter_result = (compare > 0);
    } break;
    case LIKE_TO: {
      compare = left_cell.like_match(right_cell);
      filter_result = (0 == compare);
    } break;
    case NOT_LIKE: {
      compare = left_cell.like_match(right_cell);
      filter_result = (compare != 0);
    } break;
    default: {
      LOG_WARN("invalid compare type: %d", comp);
    } break;
    }
    if (!filter_result) {
      return false;
    }
  }
  return true;
}