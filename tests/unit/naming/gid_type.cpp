////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#include <hpx/util/lightweight_test.hpp>
#include <hpx/runtime/naming/name.hpp>

int main()
{
    using hpx::naming::gid_type;

    { // constructor and retrieval (get_msb/get_lsb) tests 
      gid_type gid0(0xdeadbeefULL); // lsb ctor
      gid_type gid1(0xdeadbeefULL, 0xcededeedULL); // msb + lsb ctor 

      // check that the values were assigned and can be accessed  
      HPX_TEST_EQ(gid0.get_msb(), 0x0ULL);
      HPX_TEST_EQ(gid0.get_lsb(), 0xdeadbeefULL);
      HPX_TEST_EQ(gid1.get_msb(), 0xdeadbeefULL);
      HPX_TEST_EQ(gid1.get_lsb(), 0xcededeedULL);
    }   

    { // assignment tests
      gid_type gid(0xdeadbeefULL, 0xcededeedULL); // msb + lsb ctor 
     
      // sanity check 
      HPX_SANITY_EQ(gid.get_msb(), 0xdeadbeefULL);
      HPX_SANITY_EQ(gid.get_lsb(), 0xcededeedULL);

      gid = 0xfeedbeefULL; // lsb assignment

      HPX_TEST_EQ(gid.get_msb(), 0x0ULL); // make sure the msb got reset
      HPX_TEST_EQ(gid.get_lsb(), 0xfeedbeefULL); // make sure the lsb got set
    }
    
    { // equality/inequality tests
      gid_type gid0(0xbeefULL, 0xcedeULL),
               gid1(0xbeefULL, 0xcedeULL), // lsb == lsb, msb == msb case
               gid2(0xcedeULL), // lsb == lsb, msb != msb case 
               gid3(0xbeefULL, 0x1ULL), // lsb != lsb, msb == msb case 
               gid4(0x1ULL); // lsb != lsb, msb != msb case 
     
      // sanity check 
      HPX_SANITY_EQ(gid0.get_msb(), 0xbeefULL);
      HPX_SANITY_EQ(gid0.get_lsb(), 0xcedeULL);
      HPX_SANITY_EQ(gid1.get_msb(), 0xbeefULL);
      HPX_SANITY_EQ(gid1.get_lsb(), 0xcedeULL);
      HPX_SANITY_EQ(gid2.get_msb(), 0x0ULL);
      HPX_SANITY_EQ(gid2.get_lsb(), 0xcedeULL);
      HPX_SANITY_EQ(gid3.get_msb(), 0xbeefULL);
      HPX_SANITY_EQ(gid3.get_lsb(), 0x1ULL);
      HPX_SANITY_EQ(gid4.get_msb(), 0x0ULL);
      HPX_SANITY_EQ(gid4.get_lsb(), 0x1ULL);

      // equalities
      HPX_TEST_EQ(gid0, gid0);
      HPX_TEST_EQ(gid0, gid1);

      // inequalities
      HPX_TEST(gid0 != gid2); 
      HPX_TEST(gid0 != gid3); 
      HPX_TEST(gid0 != gid4); 
    }
 
    { // post-increment tests (post-increment should return a temporary)
      gid_type gid0(~0x0ULL), // boundary case 
               gid1(0xabULL), // < ~0x0ULL case
               gid2(0xdeULL, 0x0ULL), // 0 lsb, > ~0x0ULL case
               gid3(0xdeULL, 0xadULL), // none-zero lsb, > ~0x0ULL case
               // we need these to check the order of operations
               eq0(~0x0ULL), 
               eq1(0xabULL), 
               eq2(0xdeULL, 0x0ULL), 
               eq3(0xdeULL, 0xadULL); 
      
      // sanity checks
      HPX_SANITY_EQ(gid0.get_msb(), 0x0ULL);
      HPX_SANITY_EQ(gid0.get_lsb(), ~0x0ULL); 
      HPX_SANITY_EQ(gid1.get_msb(), 0x0ULL);
      HPX_SANITY_EQ(gid1.get_lsb(), 0xabULL); 
      HPX_SANITY_EQ(gid2.get_msb(), 0xdeULL);
      HPX_SANITY_EQ(gid2.get_lsb(), 0x0ULL); 
      HPX_SANITY_EQ(gid3.get_msb(), 0xdeULL);
      HPX_SANITY_EQ(gid3.get_lsb(), 0xadULL); 

      // the gids which are checked here should be temporaries
      HPX_TEST_EQ(gid0++, eq0);
      HPX_TEST_EQ(gid1++, eq1);
      HPX_TEST_EQ(gid2++, eq2);
      HPX_TEST_EQ(gid3++, eq3);

      //   0x0000000000000000ffffffffffffffff 
      // + 0x00000000000000000000000000000001 
      // ------------------------------------
      //   0x00000000000000010000000000000000    
      HPX_TEST_EQ(gid0.get_msb(), 0x1ULL);
      HPX_TEST_EQ(gid0.get_lsb(), 0x0ULL); 

      //   0x000000000000000000000000000000ab 
      // + 0x00000000000000000000000000000001 
      // ------------------------------------
      //   0x00000000000000de00000000000000ac    
      HPX_TEST_EQ(gid1.get_msb(), 0x0ULL);
      HPX_TEST_EQ(gid1.get_lsb(), 0xacULL); 

      //   0x00000000000000de0000000000000000 
      // + 0x00000000000000000000000000000001 
      // ------------------------------------
      //   0x00000000000000de0000000000000001    
      HPX_TEST_EQ(gid2.get_msb(), 0xdeULL);
      HPX_TEST_EQ(gid2.get_lsb(), 0x1ULL); 

      //   0x00000000000000de00000000000000ad 
      // + 0x00000000000000000000000000000001 
      // ------------------------------------
      //   0x00000000000000de00000000000000ae    
      HPX_TEST_EQ(gid3.get_msb(), 0xdeULL);
      HPX_TEST_EQ(gid3.get_lsb(), 0xaeULL); 
    }
    
    { // pre-increment tests (post-increment should return the original)
      gid_type gid0(~0x0ULL), // boundary case 
               gid1(0xabULL), // < ~0x0ULL case
               gid2(0xdeULL, 0x0ULL), // 0 lsb, > ~0x0ULL case
               gid3(0xdeULL, 0xadULL), // none-zero lsb, > ~0x0ULL case
               // we need these to check the order of operations
               eq0(~0x0ULL), 
               eq1(0xabULL), 
               eq2(0xdeULL, 0x0ULL), 
               eq3(0xdeULL, 0xadULL); 
      
      // sanity checks
      HPX_SANITY_EQ(gid0.get_msb(), 0x0ULL);
      HPX_SANITY_EQ(gid0.get_lsb(), ~0x0ULL); 
      HPX_SANITY_EQ(gid1.get_msb(), 0x0ULL);
      HPX_SANITY_EQ(gid1.get_lsb(), 0xabULL); 
      HPX_SANITY_EQ(gid2.get_msb(), 0xdeULL);
      HPX_SANITY_EQ(gid2.get_lsb(), 0x0ULL); 
      HPX_SANITY_EQ(gid3.get_msb(), 0xdeULL);
      HPX_SANITY_EQ(gid3.get_lsb(), 0xadULL); 
      
      // the gids which are checked here should be the incremented originals
      HPX_TEST_EQ(++gid0, eq0 + 1);
      HPX_TEST_EQ(++gid1, eq1 + 1);
      HPX_TEST_EQ(++gid2, eq2 + 1);
      HPX_TEST_EQ(++gid3, eq3 + 1);

      //   0x0000000000000000ffffffffffffffff 
      // + 0x00000000000000000000000000000001 
      // ------------------------------------
      //   0x00000000000000010000000000000000    
      HPX_TEST_EQ(gid0.get_msb(), 0x1ULL);
      HPX_TEST_EQ(gid0.get_lsb(), 0x0ULL); 

      //   0x000000000000000000000000000000ab 
      // + 0x00000000000000000000000000000001 
      // ------------------------------------
      //   0x00000000000000de00000000000000ac    
      HPX_TEST_EQ(gid1.get_msb(), 0x0ULL);
      HPX_TEST_EQ(gid1.get_lsb(), 0xacULL); 

      //   0x00000000000000de0000000000000000 
      // + 0x00000000000000000000000000000001 
      // ------------------------------------
      //   0x00000000000000de0000000000000001    
      HPX_TEST_EQ(gid2.get_msb(), 0xdeULL);
      HPX_TEST_EQ(gid2.get_lsb(), 0x1ULL); 

      //   0x00000000000000de00000000000000ad 
      // + 0x00000000000000000000000000000001 
      // ------------------------------------
      //   0x00000000000000de00000000000000ae    
      HPX_TEST_EQ(gid3.get_msb(), 0xdeULL);
      HPX_TEST_EQ(gid3.get_lsb(), 0xaeULL); 
    }

    { // arithmetic (operator+) tests
      gid_type gid0(~0x0ULL), // boundary case 
               gid1(0xabULL), // < ~0x0ULL case
               gid2(0xdeULL, 0x0ULL), // 0 lsb, > ~0x0ULL case
               gid3(0xdeULL, 0xadULL), // none-zero lsb, > ~0x0ULL case
               gid4(0x1ULL); // 1 case

      // sanity checks
      HPX_SANITY_EQ(gid0.get_msb(), 0x0ULL);
      HPX_SANITY_EQ(gid0.get_lsb(), ~0x0ULL); 
      HPX_SANITY_EQ(gid1.get_msb(), 0x0ULL);
      HPX_SANITY_EQ(gid1.get_lsb(), 0xabULL); 
      HPX_SANITY_EQ(gid2.get_msb(), 0xdeULL);
      HPX_SANITY_EQ(gid2.get_lsb(), 0x0ULL); 
      HPX_SANITY_EQ(gid3.get_msb(), 0xdeULL);
      HPX_SANITY_EQ(gid3.get_lsb(), 0xadULL); 
      HPX_SANITY_EQ(gid4.get_msb(), 0x0ULL);
      HPX_SANITY_EQ(gid4.get_lsb(), 0x1ULL); 

      //   0x0000000000000000ffffffffffffffff 
      // + 0x00000000000000000000000000000001 
      // ------------------------------------
      //   0x00000000000000010000000000000000    
      HPX_TEST_EQ((gid0 + gid4).get_msb(), 0x1ULL);
      HPX_TEST_EQ((gid0 + gid4).get_lsb(), 0x0ULL); 

      //   0x000000000000000000000000000000ab 
      // + 0x00000000000000000000000000000001 
      // ------------------------------------
      //   0x00000000000000de00000000000000ac    
      HPX_TEST_EQ((gid1 + gid4).get_msb(), 0x0ULL);
      HPX_TEST_EQ((gid1 + gid4).get_lsb(), 0xacULL); 

      //   0x00000000000000de0000000000000000 
      // + 0x00000000000000000000000000000001 
      // ------------------------------------
      //   0x00000000000000de0000000000000001    
      HPX_TEST_EQ((gid2 + gid4).get_msb(), 0xdeULL);
      HPX_TEST_EQ((gid2 + gid4).get_lsb(), 0x1ULL); 

      //   0x00000000000000de00000000000000ad 
      // + 0x00000000000000000000000000000001 
      // ------------------------------------
      //   0x00000000000000de00000000000000ae    
      HPX_TEST_EQ((gid3 + gid4).get_msb(), 0xdeULL);
      HPX_TEST_EQ((gid3 + gid4).get_lsb(), 0xaeULL); 

      //   0x00000000000000de00000000000000ad 
      // + 0x000000000000000000000000000000ab 
      // ------------------------------------
      //   0x00000000000000de0000000000000158 
      HPX_TEST_EQ((gid3 + gid1).get_msb(), 0xdeULL);
      HPX_TEST_EQ((gid3 + gid1).get_lsb(), 0x158ULL); 
     
      // addition should not mutate the originals 
      HPX_TEST_EQ(gid0.get_msb(), 0x0ULL);
      HPX_TEST_EQ(gid0.get_lsb(), ~0x0ULL); 
      HPX_TEST_EQ(gid1.get_msb(), 0x0ULL);
      HPX_TEST_EQ(gid1.get_lsb(), 0xabULL); 
      HPX_TEST_EQ(gid2.get_msb(), 0xdeULL);
      HPX_TEST_EQ(gid2.get_lsb(), 0x0ULL); 
      HPX_TEST_EQ(gid3.get_msb(), 0xdeULL);
      HPX_TEST_EQ(gid3.get_lsb(), 0xadULL); 
      HPX_TEST_EQ(gid4.get_msb(), 0x0ULL);
      HPX_TEST_EQ(gid4.get_lsb(), 0x1ULL); 
    }

    { // logical shift tests
      gid_type gid(~0x0ULL, ~0x0ULL);

      // sanity checks 
      HPX_SANITY_EQ(gid.get_msb(), ~0x0ULL);
      HPX_SANITY_EQ(gid.get_lsb(), ~0x0ULL);

      ++gid; // should cause a shift 

      // in C/C++
      //   0xffffffffffffffffffffffffffffffff 
      // + 0x00000000000000000000000000000001
      // ------------------------------------
      //   0x00000000000000000000000000000000
      HPX_TEST_EQ(gid.get_lsb(), 0); 
      HPX_TEST_EQ(gid.get_msb(), 0); 
  
      gid.set_msb(~0x0ULL); gid.set_lsb(~0x0ULL); 

      // in C/C++
      //   0xffffffffffffffffffffffffffffffff 
      // + 0xffffffffffffffffffffffffffffffff 
      // ------------------------------------
      //   0xfffffffffffffffffffffffffffffffe 
      HPX_TEST_EQ((gid + gid).get_lsb(), 0xfffffffffffffffeULL); 
      HPX_TEST_EQ((gid + gid).get_msb(), ~0x0ULL); 
      
      // addition should not mutate the originals 
      HPX_TEST_EQ(gid.get_msb(), ~0x0ULL);
      HPX_TEST_EQ(gid.get_lsb(), ~0x0ULL); 
    }

    { // boolean conversions
      gid_type gid; // should be default initialized to 0

      HPX_SANITY_EQ(bool(gid), false);

      // lsb == true and msb == true 
      gid.set_lsb(0xffULL);
      gid.set_msb(0xffULL);
      HPX_TEST_EQ_MSG(bool(gid), true,
        "'lsb == true' and 'msb == true' case failed");

      // lsb = false and msb == true
      gid.set_lsb(boost::uint64_t(0x0ULL));
      gid.set_msb(boost::uint64_t(0xddULL));
      HPX_TEST_EQ_MSG(bool(gid), true,
        "'lsb == false' and 'msb == true' case failed");

      // lsb = false and msb == true
      gid.set_lsb(boost::uint64_t(0xaULL));
      gid.set_msb(boost::uint64_t(0x0ULL));
      HPX_TEST_EQ_MSG(bool(gid), true,
        "'lsb == true' and 'msb == false' case failed");
    }

    return hpx::util::report_errors();
} 
