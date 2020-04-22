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

class TCTAGTrackerPerform extends Bundle {
  val MR  = Bool(INPUT)        //read meta
  val MW  = Bool(INPUT)        //update the meta
  val DR  = Bool(INPUT)        //read tag from data array
  val DW  = Bool(INPUT)        //write tag to data array //equals DWR+DWB
  val WB  = Bool(INPUT)        //write dirty line back to memory
  val F   = Bool(INPUT)        //fetch the target cache line (from memory) // equals DWB
}

class TCMEMTrackerPerform extends Bundle {
  val TTW = Bool(INPUT)        //TTW count equals TM0W/TM1W
  val TTR = Bool(INPUT)        //TTR count
  val TTR_miss = Bool(INPUT)   //TTR miss count
  val TM0R = Bool(INPUT)       //TM0R count
  val TM0R_miss = Bool(INPUT)  //TM0R miss count
  val TM1F = Bool(INPUT)       //TM1F count
  val TM1F_miss = Bool(INPUT)  //TM1F miss count (miss in tagcache)
}

class TAGCachePerform extends Bundle {
  val tcttp = new TCTAGTrackerPerform()//
  val tcmtp = new TCMEMTrackerPerform()
}

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
