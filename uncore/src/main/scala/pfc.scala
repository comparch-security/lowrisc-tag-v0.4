// See LICENSE for license details.

package uncore

import Chisel._
import cde.{Parameters, Field}

case object PFCL2N extends Field[Int]

class L1ICachePerform extends Bundle {
  val read = Bool(INPUT)
  val read_miss = Bool(INPUT)
}

class L1DCachePerform extends Bundle {
  val read = Bool(INPUT)
  val read_miss = Bool(INPUT)
  val write =Bool(INPUT)
  val write_miss = Bool(INPUT)
}

class L2DCachePerform extends Bundle {
  val read = Bool(INPUT) //inner.acquire
  val read_miss = Bool(INPUT) //outer.acquire
  val write =Bool(INPUT) //inner.release
  val write_back = Bool(INPUT) //outer.release
}
class TAGCachePerform extends L1DCachePerform

class CachePerform(implicit val p: Parameters) extends Bundle {
  val L1I = new L1ICachePerform()
  val L1D = new L1DCachePerform()
  val L2D = Vec(p(PFCL2N), new L2DCachePerform())
  val TAG = new TAGCachePerform()
}

class Perform(implicit val p: Parameters) extends Bundle
{
  val Cache = new CachePerform()
}
