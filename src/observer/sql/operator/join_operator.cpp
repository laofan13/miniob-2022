
#include "common/log/log.h"
#include "sql/operator/join_operator.h"
#include "storage/record/record.h"


RC JoinOperator::open()
{
  RC rc = left_->open();
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open left child operator: %s", strrc(rc));
    return rc;
  }

  while (left_->next() == RC::SUCCESS) {
    auto tuple = static_cast<RowTuple*>(left_->current_tuple());

    Record *record = new Record(tuple->record());
    RowTuple *row_tuple = new RowTuple(*tuple);
    row_tuple->set_record(record);
    left_record_.push_back(record);
    left_tuples_.push_back(row_tuple);
  }

  total_num_ *= left_tuples_.size();

  rc = right_->open();
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open left child operator: %s", strrc(rc));
    return rc;
  }

  while (right_->next() == RC::SUCCESS) {
    auto tuple = static_cast<RowTuple*>(right_->current_tuple());

    Record *record = new Record(tuple->record());
    RowTuple *row_tuple = new RowTuple(*tuple);
    row_tuple->set_record(record);
    right_record_.push_back(record);
    right_tuples_.push_back(row_tuple);
  }
  total_num_ *= right_tuples_.size();

  return RC::SUCCESS;
}

RC JoinOperator::next()
{
  RC rc = RC::SUCCESS;
  while(current_index_ < total_num_) {
    tuple_.clear();
    auto idx = current_index_ / right_tuples_.size();
    tuple_.add_tuple(left_tuples_[idx]);
    
    idx = current_index_ % right_tuples_.size();
    tuple_.add_tuple(right_tuples_[idx]);

    if (do_predicate(&tuple_)) {
      current_index_++;
      return rc;
    }
    current_index_++;
  }
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
  return RC::SUCCESS;
}

Tuple * JoinOperator::current_tuple()
{
  return &tuple_;
}

bool JoinOperator::do_predicate(Tuple *tuple)
{
    Expression *left_expr = filter_unit_->left();
    Expression *right_expr = filter_unit_->right();
    CompOp comp = filter_unit_->comp();
    TupleCell left_cell;
    TupleCell right_cell;

    RC rc = RC::SUCCESS;
    rc = left_expr->get_value(*tuple, left_cell);
    if(rc != RC::SUCCESS) {
      return rc;
    }
    rc = right_expr->get_value(*tuple, right_cell);
    if(rc != RC::SUCCESS) {
      return rc;
    }
    int compare = left_cell.compare(right_cell);
    return (0 == compare); 
}