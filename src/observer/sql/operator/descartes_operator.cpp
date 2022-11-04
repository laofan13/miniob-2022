#include "common/log/log.h"
#include "sql/operator/descartes_operator.h"
#include "storage/record/record.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/common/field.h"


RC DescartesOperator::open()
{
  for (auto it = scan_opers_.rbegin(); it != scan_opers_.rend(); it++)
  {
    auto &scan_oper = *it;
    std::vector<Tuple *> vec_tuples;
    std::vector<Record *> vec_records;
    RC rc = scan_oper->open();
    if (rc == RC::SUCCESS) {
      while (scan_oper->next() == RC::SUCCESS) {
        auto tuple = static_cast<RowTuple*>(scan_oper->current_tuple());

        Record *record = new Record(tuple->record());
        RowTuple *row_tuple = new RowTuple(*tuple);
        row_tuple->set_record(record);

        vec_records.push_back(record);
        vec_tuples.push_back(row_tuple);
      }
   
      index_mul_.push_back({vec_tuples.size(), total_num_});
      total_num_ *= vec_tuples.size();

      table_tuples.push_back(vec_tuples);
      table_records.push_back(vec_records);
    } else {
      return rc;
    }
  }

  return RC::SUCCESS;
}

RC DescartesOperator::next()
{
  CompositeTuple *composite_tuple = new CompositeTuple();
  if (current_index_ >= total_num_) {
    return RC::RECORD_EOF;
  }
  for (int i = 0; i < (int)table_tuples.size(); i++)
  {
    auto &vec_tuples = table_tuples[i];
    int row_index = (current_index_ / index_mul_[i].second) % index_mul_[i].first;
    auto tuple = vec_tuples[row_index];
    composite_tuple->add_tuple(tuple);
  }
  composite_table_tuples_.push_back(composite_tuple);
  tuple_ = composite_tuple;
  current_index_++;
  return RC::SUCCESS;
}

RC DescartesOperator::close()
{
  RC rc = RC::SUCCESS;
  for (auto scan_oper : scan_opers_) {
    if ((rc = scan_oper->close()) != RC::SUCCESS) {
      return rc;
    }
  }
  return rc;
}

Tuple * DescartesOperator::current_tuple()
{
  return tuple_;
}