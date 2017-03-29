/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   Header file containing executor class that runs suite.
 *
 ******************************************************************************
 */

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2017, Lawrence Livermore National Security, LLC.
//
// Produced at the Lawrence Livermore National Laboratory
//
// LLNL-CODE-xxxxxx
//
// All rights reserved.
//
// This file is part of the RAJA Performance Suite.
//
// For additional details, please read the file LICENSE.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//


#ifndef RAJAPerf_Executor_HXX
#define RAJAPerf_Executor_HXX

#include "common/RAJAPerfSuite.hxx"
#include "common/RunParams.hxx"

#include <iosfwd>
#include <utility>
#include <set>

namespace rajaperf {

class KernelBase;

class Executor
{
public:
  Executor( int argc, char** argv );

  ~Executor();

  void setupSuite();

  void reportRunSummary(std::ostream& str) const;

  void runSuite();
  
  void outputRunData();

private:
  Executor() = delete;

  enum CSVRepMode {
    Timing = 0,
    Speedup,

    NumRepModes // Keep this one last and DO NOT remove (!!)
  };

  struct FOMGroup {
    VariantID baseline;
    std::vector<VariantID> variants;
  }; 

  bool haveReferenceVariant() { return reference_vid < NumVariants; }

  void writeCSVReport(const std::string& filename, CSVRepMode mode);
  std::string getReportTitle(CSVRepMode mode);
  long double getReportDataEntry(CSVRepMode mode, 
                                 KernelBase* kern, VariantID vid);

  void writeChecksumReport(const std::string& filename);  

  void writeFOMReport(const std::string& filename);
  void getFOMGroups(std::vector<FOMGroup>& fom_groups);
  
  RunParams run_params;
  std::vector<KernelBase*> kernels;  
  std::vector<VariantID>   variant_ids;

  VariantID reference_vid;

};

}  // closing brace for rajaperf namespace

#endif  // closing endif for header file include guard