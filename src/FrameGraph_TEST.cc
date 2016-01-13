/*
 * Copyright (C) 2015 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <gtest/gtest.h>
#include <thread>

#include "ignition/math/Helpers.hh"
#include "ignition/math/FrameGraph.hh"

using namespace ignition;
using namespace math;

// LocalPose with string
// CreateRelative pose
// DeleteFrame
// AccessFrame relative version

/////////////////////////////////////////////////
TEST(FrameGraphTest, AbsolutePaths)
{
  // store result of various calls
  bool r;

  // frameGraph comes with a built-in "root" frame
  FrameGraph frameGraph;

  Pose3d pa(1, 0, 0, 0, 0, 0);

  // this path's parent is incorrect ("root has no /")
  EXPECT_THROW(frameGraph.AddFrame("root", "x", pa), FrameException);

  // # is not a good name
  EXPECT_THROW(frameGraph.AddFrame("root", "#", pa), FrameException);
  // '' is not a good name
  EXPECT_THROW(frameGraph.AddFrame("", "ho", pa), FrameException);
  // '' as a name
  EXPECT_THROW(frameGraph.AddFrame("/", "", pa), FrameException);

  // this path is not fully qualified
  EXPECT_THROW(frameGraph.AddFrame("/universe", "x", pa), FrameException);

  // this path is not fully qualified because of ".."
  EXPECT_THROW(frameGraph.AddFrame("/..", "x", pa), FrameException);

  // this path as an undefined "unknown" frame
  EXPECT_THROW(frameGraph.AddFrame("/unknown", "x", pa), FrameException);

  // this path as an illegal "!" frame
  EXPECT_THROW(frameGraph.AddFrame("/!", "x", pa), FrameException);

  // very stupid attempt at getting pose info from inexistent frame
  Pose3d p;
  EXPECT_THROW(p = frameGraph.Pose("/x", "/"), FrameException);
  EXPECT_THROW(p = frameGraph.Pose("/", "/x"), FrameException);

  // Finally, this path adds a to the built in "/" frame
  frameGraph.AddFrame("/", "a", pa);

  // try to add duplicate frame
  EXPECT_THROW(frameGraph.AddFrame("/", "a",  pa), FrameException);

  Pose3d a2w;
  // skillful pose inquiry
  a2w = frameGraph.Pose("/a", "/");
  EXPECT_EQ(pa, a2w);

  // the local pose of a relative to its parent is also pa
  Pose3d localPose = frameGraph.LocalPose("/a");
  EXPECT_EQ(pa, localPose);

  // error: x does not exist
  EXPECT_THROW(p = frameGraph.Pose("/a", "/x"), FrameException);

  // add b
  Pose3d pb(0, 1, 0, 0, 0, 0);
  frameGraph.AddFrame("/", "b", pb);

  // Tests using relative paths
  Pose3d w2b = frameGraph.Pose("/b", "..");
  EXPECT_EQ(pb, w2b);

  // using '.'
  Pose3d b2b = frameGraph.Pose("/b", ".");
  EXPECT_EQ(b2b, Pose3d(0, 0, 0, 0, 0, 0));

  // using ''
  EXPECT_THROW(frameGraph.Pose("/b", ""), FrameException);
  EXPECT_THROW(frameGraph.Pose("/b", "?"), FrameException);

  // Relative path from b to a
  Pose3d b2a, b2a2;
  b2a = frameGraph.Pose("/a", "/b");
  b2a2 = frameGraph.Pose("/a", "../b");
  EXPECT_EQ(b2a, b2a2);
}

/////////////////////////////////////////////////
TEST(FrameGraphTest, FrameChildren)
{
  Pose3d pa(1, 0, 0, 0, 0, 0);
  Pose3d paa(0, 1, 0, 0, 0, 0);
  Pose3d paaa(0, 0, 0, 0, 1.570790, 0);
  Pose3d paaaa(0, 0, 1, 0, 1.570790, 0);

  FrameGraph frameGraph;
  frameGraph.AddFrame("/", "a", pa);
  frameGraph.AddFrame("/a", "aa", paa);
  frameGraph.AddFrame("/a/aa", "aaa", paaa);
  frameGraph.AddFrame("/a/aa", "aaaa", paaaa);

  FrameWeakPtr frame = frameGraph.Frame("/a");
  FramePtr f = frame.lock();
  EXPECT_TRUE(f != NULL);
  EXPECT_EQ(f->Children().size(), 1);

  frame = frameGraph.Frame("/a/aa");
  f = frame.lock();
  EXPECT_TRUE(f != NULL);
  EXPECT_EQ(f->Children().size(), 2);

  int index = 0;
  for (auto const &c : f->Children())
  {
    if (index == 0)
      EXPECT_EQ(c.first, "aaa");
    else
      EXPECT_EQ(c.first, "aaaa");
    ++index;
  }

  EXPECT_TRUE(f->HasChild("aaaa"));
  EXPECT_FALSE(f->AddChild("aaaa", paa, frameGraph.Frame("/a/aa")));
  EXPECT_FALSE(f->DeleteChild("aaaaa"));
  EXPECT_FALSE(f->HasChild("a"));
}

/////////////////////////////////////////////////
TEST(FrameGraphTest, DeleteFrame)
{
  Pose3d pa(1, 0, 0, 0, 0, 0);
  Pose3d paa(0, 1, 0, 0, 0, 0);
  Pose3d paaa(0, 0, 0, 0, 1.570790, 0);

  FrameGraph frameGraph;
  frameGraph.AddFrame("/", "a", pa);
  frameGraph.AddFrame("/a", "aa", paa);
  frameGraph.AddFrame("/a/aa", "aaa", paaa);

  // not an absolute path
  EXPECT_THROW(frameGraph.DeleteFrame(".."), FrameException);
  // not a real path
  EXPECT_THROW(frameGraph.DeleteFrame("/banana"), FrameException);

  frameGraph.DeleteFrame("/a");
  EXPECT_THROW(frameGraph.AddFrame("/a/aa", "aaa", paaa), FrameException);
}

/////////////////////////////////////////////////
// this tests adds coverage
// of Frame and RelativePose
TEST(FrameGraphTest, CopyFrames)
{
  Pose3d pa(1, 0, 0, 0, 0, 0);
  Pose3d paa(0, 1, 0, 0, 0, 0);
  Pose3d paaa(0, 0, 0, 0, 1.570790, 0);

  FrameGraph frameGraph;
  frameGraph.AddFrame("/", "a", pa);
  frameGraph.AddFrame("/a", "aa", paa);
  frameGraph.AddFrame("/a/aa", "aaa", paaa);

  auto frame1 = frameGraph.Frame("/a");
  {
    auto f = frame1.lock();
    EXPECT_EQ(f->Name(), "a");
  }
  auto rel = frameGraph.CreateRelativePose("/a/aa", "/");
  Pose3d p;

  RelativePose *rp = new RelativePose(rel);
  delete rp;

  p = frameGraph.Pose(rel);
  EXPECT_EQ(p, Pose3d(1, 1, 0, 0, 0, 0));

  frameGraph.DeleteFrame("/a");
  EXPECT_THROW(frameGraph.AddFrame("/a/aa", "aaa", paaa), FrameException);
}


/////////////////////////////////////////////////
std::string p2str(const Pose3d &_p)
{
  std::stringstream ss;
  auto p = _p.Pos();
  ss << "("<< p.X() << ", " << p.Y() << ", " << p.Z() << ", ";
  auto r =  _p.Rot().Euler();
  ss << r.X() << ", " <<  r.Y() << ", " << r.Z() << ")";
  return ss.str();
}

std::string link(const Pose3d &_p0, const Pose3d &_p1)
{
  std::stringstream ss;
  auto p0 = _p0.Pos();
  auto p1 = _p1.Pos();
  ss << "link(" << p0.X() << ", " << p0.Y() << ", " << p0.Z()
     << ", " << p1.X() << ", " << p1.Y() << ", " << p1.Z() << ")";
  return ss.str();
}

/////////////////////////////////////////////////
TEST(FrameGraphTest, coverage)
{
  // this test is only used for coverage
  auto x = new FrameException("bad");
  delete x;

  FrameGraph frameGraph;
  frameGraph.AddFrame("/", "a", Pose3d(0, 0, 0, 0, 0, 0));
  auto f = frameGraph.Frame("/a");
  // exercises the '.'
  auto f2 = frameGraph.Frame(f, ".././a");
  // now we remove the framm
  frameGraph.DeleteFrame("/a");
  // try to access deleted frame
  EXPECT_THROW(frameGraph.LocalPose("/a"), FrameException);
  EXPECT_THROW(frameGraph.LocalPose(f), FrameException);
}

/////////////////////////////////////////////////
TEST(FrameGraphTest, Pose1)
{
  //
  //          ---root ---
  //          |          |
  //          a          b

  Pose3d w;  // root
  Pose3d pa(10, 0, 0, 0, 0, 0);
  Pose3d pb(0, 10, 0, 0, 0, 0);

  std::cout << "\n";
  FrameGraph frameGraph;
  frameGraph.AddFrame("/", "a", pa);
  frameGraph.AddFrame("/", "b", pb);

  // pose of a from the root's perspective
  Pose3d pwa = frameGraph.Pose("/a", "/");

  Pose3d pwb = frameGraph.Pose("/b", "/");
  Pose3d pwab = frameGraph.Pose("/a", "/b");
  Pose3d pwba = frameGraph.Pose("/b", "/a");

  std::cout << "pose" << p2str(pwa) << ";  // absolute a" << std::endl;
  std::cout << "pose" << p2str(pwb) << ";  // absolute b" << std::endl;
  std::cout << "pose" << p2str(pwab) << ";  // absolute ab" << std::endl;
  std::cout << "pose" << p2str(pwba) << ";  // absolute ba" << std::endl;

  // a expressed in b
  EXPECT_EQ(pwab, Pose3d(10, -10, 0, 0, 0, 0));
  //  expressed in a
  EXPECT_EQ(pwba, Pose3d(-10, 10, 0, 0, 0, 0));

  // now rotate a 90 degrees around z
  frameGraph.SetLocalPose("/a", Pose3d(10, 0, 0, 0, 0, 1.5707));
  pwa = frameGraph.Pose("/a", "/");
  pwb = frameGraph.Pose("/b", "/");
  pwab = frameGraph.Pose("/a", "/b");
  pwba = frameGraph.Pose("/b", "/a");

  // a expressed in b
  EXPECT_EQ(pwab, Pose3d(10, -10, 0, 0, 0, 1.5707));
  // b expressed in a
  EXPECT_EQ(pwba, Pose3d(10, 10, 0, 0, 0, -1.5707));
}


/////////////////////////////////////////////////
TEST(FrameGraphTest, RelativePose)
{
  //             root
  //              |
  //              a
  //              |
  //          --------
  //          |      |
  //          aa     ab

  Pose3d w;  // root
  Pose3d pa(10, 0, 0, 0, 0, 0);
  Pose3d paa(10, 0, 0, 0, 0, 0);
  Pose3d pab(0, 10, 0, 0, 0, 0);

  std::cout << "\n";
  FrameGraph frameGraph;
  frameGraph.AddFrame("/", "a", pa);
  frameGraph.AddFrame("/a", "aa", paa);
  frameGraph.AddFrame("/a", "ab", pab);

  // rotate 30 degrees
  double angle = 0.523599;
  // (10, 0, 0, angle, angle, 0);
  Pose3d p(10, 0, 0, 0, 0, angle);
  frameGraph.SetLocalPose("/a", p);
  std::cout << "/ a local pose: " << p << "\n";

  Pose3d pwa = frameGraph.Pose("/a", "/");
  Pose3d pwaa = frameGraph.Pose("/a/aa", "/");
  Pose3d pwab = frameGraph.Pose("/a/ab", "/");

  std::cout << "pose" << p2str(pwa) << ";  // absolute a" << std::endl;
  std::cout << "pose" << p2str(pwaa) << ";  // absolute aa" << std::endl;
  std::cout << "pose" << p2str(pwab) << ";  // absolute ab" << std::endl;
  // links
  std::cout << "color([1,1,0])" << link(w, pwa) << "; // w to a"<< std::endl;
  std::cout << "color([1,0,0])" << link(pa, pwaa)
    << "; // a to aa" << std::endl;
  std::cout << "color([0,1,0])" << link(pa, pwab)
    << "; // a to ab" << std::endl;
  std::cout << std::endl;
  auto paa2ab = frameGraph.Pose("/a/aa", "/a/ab");
  std::cout << "pose" << p2str(paa2ab) << ";  // aa to ab" << std::endl;
  std::cout << std::endl;
}


/////////////////////////////////////////////////
TEST(FrameGraphTest, RelativePaths)
{
  //             root
  //              |
  //              a
  //              |
  //          --------
  //          |      |
  //          aa     ab
  //          |
  //         aaa
  //
  // In this test we rotate a around z and check that
  // aa and ab remain fixed from each other...
  // also a lot of output to help debugging
  Pose3d pa(10, 0, 0, 0, 0, 0);
  Pose3d paa(10, 0, 0, 0, 0, 0);
  Pose3d paaa(10, 0, 0, 0, 0, 0);
  Pose3d pab(0, 10, 0, 0, 0, 0);

  FrameGraph frameGraph;
  frameGraph.AddFrame("/", "a", pa);
  frameGraph.AddFrame("/a", "aa", paa);
  frameGraph.AddFrame("/a/aa", "aaa", paaa);
  frameGraph.AddFrame("/a", "ab", pab);

  double sweep = 2 * 3.1415926;
  // Let's vary the local pose of aa with a rotation, this should
  //  move aaa in the root
  Pose3d w(0, 0, 0, 0, 0, 0);
  unsigned int steps = 10;
  for (unsigned int i = 0; i < (steps + 1); ++i)
  {
    auto angle = i * (sweep / steps);
    // (10, 0, 0, angle, angle, 0);
    Pose3d p(10, 0, 0, 0, 0, angle);
    frameGraph.SetLocalPose("/a", p);

    auto pa = frameGraph.Pose("/a", "/");
    std::cout << "pose" << p2str(pa) << "; // a" << std::endl;
    auto paa = frameGraph.Pose("/a/aa", "/");
    std::cout << "pose" << p2str(paa) << "; // aa" << std::endl;
    auto paaa = frameGraph.Pose("/a/aa/aaa", "/");
    std::cout << "pose" << p2str(paaa) << ";  // aaa" << std::endl;
    auto pab = frameGraph.Pose("/a/ab", "/");
    std::cout << "pose" << p2str(pab) << ";  // ab" << std::endl;
    auto paa2ab = frameGraph.Pose("/a/aa", "/a/ab");
    std::cout << "pose" << p2str(paa2ab) << ";  // aa to ab" << std::endl;
    std::cout << link(w, pa) << "; // root to a " << std::endl;
    std::cout << link(pa, paa) << "; // a to aa" << std::endl;
    std::cout << link(paa, paaa) << "; // aa to aaa" << std::endl;
    std::cout << link(pa, pab) << "; // a to ab" << std::endl;
    std::cout << std::endl;

    EXPECT_EQ(paa2ab, Pose3d(10, -10, 0, 0, 0, 0));
  }
}

/////////////////////////////////////////////////
TEST(FrameGraphTest, SimplePose)
{
  // In a graph with a single frame, the pose of
  // the frame should be the same as the relative
  // pose between the frame and the root
  FrameGraph frameGraph;

  Pose3d pa(1, 0, 0, 0, 0, 0);
  frameGraph.AddFrame("/", "a", pa);

  Pose3d r;
  r = frameGraph.Pose("/a", "/");
  EXPECT_EQ(pa, r);

  Pose3d pb(2, 0, 0, 0, 0, 0);
  frameGraph.SetLocalPose("/a", pb);

  EXPECT_EQ(pb, frameGraph.Pose("/a", "/"));
}


/////////////////////////////////////////////////
// this code executes from a thread
// it changes the local pose of a frame
void asyncStuff(FrameGraph &frameGraph)
{
  const auto &frame = frameGraph.Frame("/a");
  for (int i = 0; i < 1000001; ++i)
  {
    Pose3d p(i, 0, 0, 0, 0, 0);
    frameGraph.SetLocalPose(frame, p);
  }
}

/////////////////////////////////////////////////
TEST(FrameGraphTest, Multithreads)
{
  // In a graph with a single frame, the pose of
  // the frame should be the same as the relative
  // pose between the frame and the root
  FrameGraph frameGraph;

  Pose3d pa(0, 0, 0, 0, 0, 0);
  frameGraph.AddFrame("/", "a", pa);

  Pose3d r;
  r = frameGraph.Pose("/a", "/");
  EXPECT_EQ(pa, r);

  // a thread pool
  std::vector<std::thread> pool;
  // starting only one thread, otherswise there are errors
  for (int i = 0; i < 1; ++i)
  {
    pool.push_back(std::thread {asyncStuff, std::ref(frameGraph)});
  }
  const auto &frame = frameGraph.Frame("/a");
  EXPECT_EQ(pa, frameGraph.LocalPose(frame));
  auto rel = frameGraph.CreateRelativePose("/a", "/");
  Pose3d last = frameGraph.Pose(rel);
  for (int i = 0; i < 1000; ++i)
  {
    Pose3d p;
    p = frameGraph.Pose(rel);
    EXPECT_GE(p.Pos().X(), last.Pos().X());
    last = p;
  }
  for (auto &thread : pool)
  {
    thread.join();
  }
  // copy constructor
  auto rel2 = rel;
  EXPECT_EQ(frameGraph.Pose(rel2), frameGraph.Pose(rel));
  // empty ctor
  RelativePose rel3;
  // assignment
  rel3 = rel2;
  // assignement to self (does nothing)
  rel3 = rel3;
  EXPECT_EQ(frameGraph.Pose(rel), frameGraph.Pose(rel3));

  Pose3d p = frameGraph.Pose(rel);
  EXPECT_EQ(p, frameGraph.Pose("/a", "/"));
}

/////////////////////////////////////////////////
TEST(FrameGraphTest, Print)
{
  FrameGraph frameGraph;
  frameGraph.AddFrame("/", "a1", Pose3d(0, 0, 0, 0, 0, 0));
  frameGraph.AddFrame("/a1", "b1", Pose3d(0, 1, 0, 0, 0, 0));
  frameGraph.AddFrame("/a1", "b2", Pose3d(0, 0, 1, 0, 0, 0));
  frameGraph.AddFrame("/a1/b2", "c1", Pose3d(0, 0, 1, 0, 0, 0));
  frameGraph.AddFrame("/a1/b2", "c2", Pose3d(0, 0, 0, 1, 0, 0));
  frameGraph.AddFrame("/a1/b2/c1", "d1", Pose3d(0, 0, 0, 1, 0, 0));
  frameGraph.AddFrame("/", "a2", Pose3d(0, 0, 0, 0, 1, 0));

  std::ostringstream stream;
  stream << frameGraph;

  std::string str = R"(/ [0 0 0 0 -0 0]
/a1 [0 0 0 0 -0 0]
/a1/b1 [0 1 0 0 -0 0]
/a1/b2 [0 0 1 0 -0 0]
/a1/b2/c1 [0 0 1 0 -0 0]
/a1/b2/c1/d1 [0 0 0 1 -0 0]
/a1/b2/c2 [0 0 0 1 -0 0]
/a2 [0 0 0 0 1 0]
)";
  EXPECT_EQ(str, stream.str());
}

/////////////////////////////////////////////////
TEST(FrameGraphTest, Cycle)
{
  FrameGraph frameGraph;
  frameGraph.AddFrame("/", "a", Pose3d::Zero);
  frameGraph.AddFrame("/", "a", Pose3d::Zero);
}