/*
// @HEADER
// ***********************************************************************
//
//          Tpetra: Templated Linear Algebra Services Package
//                 Copyright (2008) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// ************************************************************************
// @HEADER
*/

// Creates vectors with different maps; tests results of export into them
// Bug7758 indicates that incorrect results of export are produced when
// the source map is NOT a superset of the target map #7758
// Analogous tests for Epetra are in epetra/test/MultiVector/Bug7758.cpp

#include "Tpetra_Core.hpp"
#include "Tpetra_Map.hpp"
#include "Tpetra_Vector.hpp"
#include "Teuchos_Array.hpp"

#include "Teuchos_UnitTestHarness.hpp"
#include "TpetraCore_ETIHelperMacros.h"

namespace {


//////////////////////////////////////////////////////////////////////////////
TEUCHOS_UNIT_TEST_TEMPLATE_4_DECL(Bug7758, DefaultToDefault, Scalar,LO,GO,Node)
{
  Teuchos::RCP<const Teuchos::Comm<int> > comm = Tpetra::getDefaultComm();
  int me = comm->getRank();
  int np = comm->getSize();
  int ierr = 0;

  Teuchos::FancyOStream foo(Teuchos::rcp(&std::cout,false));

  using vector_t = Tpetra::Vector<Scalar,LO,GO,Node>;
  using map_t = Tpetra::Map<LO,GO,Node>;

  const size_t nGlobalEntries = 8 * np;
  const Scalar tgtScalar = 100. * (me+1);
  const Scalar srcScalar = 2.;
  Teuchos::Array<GO> myEntries(nGlobalEntries); 

  // Default one-to-one linear block map in Trilinos

  Teuchos::RCP<const map_t> defaultMap = 
           rcp(new map_t(nGlobalEntries, 0, comm));

  std::cout << me << " DEFAULT MAP" << std::endl;
  defaultMap->describe(foo, Teuchos::VERB_EXTREME);

  // Create vectors; see what the result is with CombineMode=ADD

  vector_t defaultVecTgt(defaultMap);
  defaultVecTgt.putScalar(tgtScalar);

  vector_t defaultVecSrc(defaultMap);
  defaultVecSrc.putScalar(srcScalar);

  // Export Default-to-default:  should be a copy of src to tgt

  Tpetra::Export<LO,GO,Node> defaultToDefault(defaultMap, defaultMap);
  defaultVecTgt.doExport(defaultVecSrc, defaultToDefault, Tpetra::ADD);

  std::cout << me << " DEFAULT TO DEFAULT " << std::endl;
  defaultVecTgt.describe(foo, Teuchos::VERB_EXTREME);

  // Check result; all vector entries should be srcScalar
  auto data = defaultVecTgt.getLocalViewHost();
  for (size_t i = 0; i < defaultVecTgt.getLocalLength(); i++)
    if (data(i,0) != srcScalar) ierr++;
  if (ierr > 0) 
    std::cout << "TEST FAILED:  DEFAULT-TO-DEFAULT TEST HAD " << ierr 
              << " FAILURES ON RANK " << me << std::endl;

  int gerr;
  Teuchos::reduceAll<int,int>(*comm, Teuchos::REDUCE_SUM, 1, &ierr, &gerr);

  TEST_ASSERT(gerr == 0);
}

//////////////////////////////////////////////////////////////////////////////
TEUCHOS_UNIT_TEST_TEMPLATE_4_DECL(Bug7758, CyclicToDefault, Scalar,LO,GO,Node)
{
  Teuchos::RCP<const Teuchos::Comm<int> > comm = Tpetra::getDefaultComm();
  int me = comm->getRank();
  int np = comm->getSize();
  int ierr = 0;

  Teuchos::FancyOStream foo(Teuchos::rcp(&std::cout,false));

  using vector_t = Tpetra::Vector<Scalar,LO,GO,Node>;
  using map_t = Tpetra::Map<LO,GO,Node>;

  const size_t nGlobalEntries = 8 * np;
  const Scalar tgtScalar = 100. * (me+1);
  const Scalar srcScalar = 2.;
  Teuchos::Array<GO> myEntries(nGlobalEntries); 

  // Default one-to-one linear block map in Trilinos

  Teuchos::RCP<const map_t> defaultMap = 
           rcp(new map_t(nGlobalEntries, 0, comm));

  std::cout << me << " DEFAULT MAP" << std::endl;
  defaultMap->describe(foo, Teuchos::VERB_EXTREME);

  // One-to-one cyclic map:  deal out entries like cards

  int nMyEntries = 0;
  for (size_t i = 0; i < nGlobalEntries; i++) {
    if (int(i % np) == me) {
      myEntries[nMyEntries++] = i;
    }
  }

  Tpetra::global_size_t dummy =
          Teuchos::OrdinalTraits<Tpetra::global_size_t>::invalid();
  Teuchos::RCP<const map_t> cyclicMap = 
           rcp(new map_t(dummy, myEntries(0,nMyEntries), 0, comm));

  std::cout << me << " CYCLIC MAP" << std::endl;
  cyclicMap->describe(foo, Teuchos::VERB_EXTREME);

  // Create vectors; see what the result is with CombineMode=ADD

  vector_t defaultVecTgt(defaultMap);
  defaultVecTgt.putScalar(tgtScalar);

  vector_t cyclicVecSrc(cyclicMap);
  cyclicVecSrc.putScalar(srcScalar);

  // Export Cyclic-to-default

  Tpetra::Export<LO,GO,Node> cyclicToDefault(cyclicMap, defaultMap);
  defaultVecTgt.doExport(cyclicVecSrc, cyclicToDefault, Tpetra::ADD);

  std::cout << me << " CYCLIC TO DEFAULT " << std::endl;
  defaultVecTgt.describe(foo, Teuchos::VERB_EXTREME);

  // Check result; all vector entries should be srcScalar

  auto data = defaultVecTgt.getLocalViewHost();
  for (size_t i = 0; i < defaultVecTgt.getLocalLength(); i++)
    if (data(i,0) != srcScalar) ierr++;
  if (ierr > 0) 
    std::cout << "TEST FAILED:  CYCLIC-TO-DEFAULT TEST HAD " << ierr 
              << " FAILURES ON RANK " << me << std::endl;

  int gerr;
  Teuchos::reduceAll<int,int>(*comm, Teuchos::REDUCE_SUM, 1, &ierr, &gerr);

  TEST_ASSERT(gerr == 0);
}

//////////////////////////////////////////////////////////////////////////////
TEUCHOS_UNIT_TEST_TEMPLATE_4_DECL(Bug7758, OverlapToDefault, Scalar,LO,GO,Node)
{
  Teuchos::RCP<const Teuchos::Comm<int> > comm = Tpetra::getDefaultComm();
  int me = comm->getRank();
  int np = comm->getSize();
  int ierr = 0;

  if (np > 1) {  // Need more than one proc to avoid duplicate entries in maps
    Teuchos::FancyOStream foo(Teuchos::rcp(&std::cout,false));

    using vector_t = Tpetra::Vector<Scalar,LO,GO,Node>;
    using map_t = Tpetra::Map<LO,GO,Node>;

    const size_t nGlobalEntries = 8 * np;
    const Scalar tgtScalar = 100. * (me+1);
    const Scalar srcScalar = 2.;
    Teuchos::Array<GO> myEntries(nGlobalEntries); 

    // Default one-to-one linear block map in Trilinos

    Teuchos::RCP<const map_t> defaultMap = 
             rcp(new map_t(nGlobalEntries, 0, comm));

    std::cout << me << " DEFAULT MAP" << std::endl;
    defaultMap->describe(foo, Teuchos::VERB_EXTREME);

    // Overlap map; some entries are stored on two procs
    int nMyEntries = 0;
    for (size_t i = 0; i < defaultMap->getNodeNumElements()/2; i++) {
      myEntries[nMyEntries++] = defaultMap->getGlobalElement(i);
    }
    for (size_t i = 0; i < defaultMap->getNodeNumElements(); i++) {
      myEntries[nMyEntries++] =
        (defaultMap->getMaxGlobalIndex() + 1 + i) % nGlobalEntries;
    }
  
    Tpetra::global_size_t dummy = 
            Teuchos::OrdinalTraits<Tpetra::global_size_t>::invalid();
    Teuchos::RCP<const map_t> overlapMap = 
             rcp(new map_t(dummy, myEntries(0,nMyEntries), 0, comm));
  
    std::cout << me << " OVERLAP MAP" << std::endl;
    overlapMap->describe(foo, Teuchos::VERB_EXTREME);

    // Create vectors; see what the result is with CombineMode=ADD

    vector_t defaultVecTgt(defaultMap);
    defaultVecTgt.putScalar(tgtScalar);

    vector_t overlapVecSrc(overlapMap);
    overlapVecSrc.putScalar(srcScalar);

    // Export Overlap-to-default

    Tpetra::Export<LO,GO,Node> overlapToDefault(overlapMap, defaultMap);
    defaultVecTgt.doExport(overlapVecSrc, overlapToDefault, Tpetra::ADD);

    std::cout << me << " OVERLAP TO DEFAULT " << std::endl;
    defaultVecTgt.describe(foo, Teuchos::VERB_EXTREME);

    auto data = defaultVecTgt.getLocalViewHost();
    for (size_t i = 0; i < defaultVecTgt.getLocalLength()/2; i++)
      if (data(i,0) != srcScalar+srcScalar) ierr++;  // overlapped
    for (size_t i = defaultVecTgt.getLocalLength()/2;
             i < defaultVecTgt.getLocalLength(); i++)
      if (data(i,0) != srcScalar) ierr++;  // not overlapped
    if (ierr > 0) 
      std::cout << "TEST FAILED:  OVERLAP-TO-DEFAULT TEST HAD " << ierr 
                << " FAILURES ON RANK " << me << std::endl;
  }

  int gerr;
  Teuchos::reduceAll<int,int>(*comm, Teuchos::REDUCE_SUM, 1, &ierr, &gerr);

  TEST_ASSERT(gerr == 0);
}

//////////////////////////////////////////////////////////////////////////////
TEUCHOS_UNIT_TEST_TEMPLATE_4_DECL(Bug7758, SupersetToDefault, Scalar,LO,GO,Node)
{
  Teuchos::RCP<const Teuchos::Comm<int> > comm = Tpetra::getDefaultComm();
  int me = comm->getRank();
  int np = comm->getSize();
  int ierr = 0;

  if (np > 1) {  // Need more than one proc to avoid duplicate entries in maps
    Teuchos::FancyOStream foo(Teuchos::rcp(&std::cout,false));

    using vector_t = Tpetra::Vector<Scalar,LO,GO,Node>;
    using map_t = Tpetra::Map<LO,GO,Node>;

    const size_t nGlobalEntries = 8 * np;
    const Scalar tgtScalar = 100. * (me+1);
    const Scalar srcScalar = 2.;
    Teuchos::Array<GO> myEntries(nGlobalEntries); 

    // Default one-to-one linear block map in Trilinos

    Teuchos::RCP<const map_t> defaultMap = 
             rcp(new map_t(nGlobalEntries, 0, comm));

    std::cout << me << " DEFAULT MAP" << std::endl;
    defaultMap->describe(foo, Teuchos::VERB_EXTREME);

    // Superset map; some entries are stored on two procs
    int nMyEntries = 0;
    for (size_t i = 0; i < defaultMap->getNodeNumElements(); i++) {
      myEntries[nMyEntries++] = defaultMap->getGlobalElement(i);
    }
    for (size_t i = 0; i < defaultMap->getNodeNumElements()/2; i++) {
      myEntries[nMyEntries++] =
        (defaultMap->getMaxGlobalIndex() + 1 + i) % nGlobalEntries;
    }
  
    Tpetra::global_size_t dummy = 
            Teuchos::OrdinalTraits<Tpetra::global_size_t>::invalid();
    Teuchos::RCP<const map_t> supersetMap =
             rcp(new map_t(dummy, myEntries(0,nMyEntries), 0, comm));
  
    std::cout << me << " SUPERSET MAP" << std::endl;
    supersetMap->describe(foo, Teuchos::VERB_EXTREME);

    // Create vectors; see what the result is with CombineMode=ADD

    vector_t defaultVecTgt(defaultMap);
    defaultVecTgt.putScalar(tgtScalar);

    vector_t supersetVecSrc(supersetMap);
    supersetVecSrc.putScalar(srcScalar);

    // Export Superset-to-default

    Tpetra::Export<LO,GO,Node> supersetToDefault(supersetMap, defaultMap);
    defaultVecTgt.doExport(supersetVecSrc, supersetToDefault, Tpetra::ADD);

    std::cout << me << " SUPERSET TO DEFAULT " << std::endl;
    defaultVecTgt.describe(foo, Teuchos::VERB_EXTREME);

    auto data = defaultVecTgt.getLocalViewHost();
    for (size_t i = 0; i < defaultVecTgt.getLocalLength()/2; i++)
      if (data(i,0) != srcScalar+srcScalar) ierr++;  // overlapped
    for (size_t i = defaultVecTgt.getLocalLength()/2;
             i < defaultVecTgt.getLocalLength(); i++)
      if (data(i,0) != srcScalar) ierr++;  // not overlapped
    if (ierr > 0) 
      std::cout << "TEST FAILED:  SUPERSET-TO-DEFAULT TEST HAD " << ierr 
                << " FAILURES ON RANK " << me << std::endl;
  }

  int gerr;
  Teuchos::reduceAll<int,int>(*comm, Teuchos::REDUCE_SUM, 1, &ierr, &gerr);

  TEST_ASSERT(gerr == 0);
}


#define UNIT_TEST_GROUP( SCALAR, LO, GO, NODE ) \
  TEUCHOS_UNIT_TEST_TEMPLATE_4_INSTANT(Bug7758, DefaultToDefault, SCALAR, LO, GO, NODE) \
  TEUCHOS_UNIT_TEST_TEMPLATE_4_INSTANT(Bug7758, CyclicToDefault, SCALAR, LO, GO, NODE) \
  TEUCHOS_UNIT_TEST_TEMPLATE_4_INSTANT(Bug7758, OverlapToDefault, SCALAR, LO, GO, NODE) \
  TEUCHOS_UNIT_TEST_TEMPLATE_4_INSTANT(Bug7758, SupersetToDefault, SCALAR, LO, GO, NODE)

  TPETRA_ETI_MANGLING_TYPEDEFS()

  TPETRA_INSTANTIATE_TESTMV( UNIT_TEST_GROUP )

} // namespace (anonymous)

