// See LICENSE for license details.

package uncore

import Chisel._
import cde.{Parameters, Field}

case object PFCL2N extends Field[Int]

trait HasPFCParameters extends HasTagParameters {
  implicit val p: Parameters
  val L2DBanks = p(PFCL2N)
}

abstract class PFCModule(implicit val p: Parameters) extends Module with HasPFCParameters
abstract class PFCBundle(implicit val p: Parameters) extends Bundle with HasPFCParameters

object PerFormanceCounter {
  def apply(cond: Bool, n: Int): UInt = {
    val c = new Counter(n)
    var wrap: Bool = null
    when (cond) { wrap = c.inc() }
    c.value
  }
}

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

class L2DCachePerformCounter(w: Int=1) extends Bundle {
  val read       = UInt(width=w) //inner.acquire
  val read_miss  = UInt(width=w) //outer.acquire
  val write      = UInt(width=w) //inner.release
  val write_back = UInt(width=w) //outer.release
}

class TCTAGTrackerPerform extends Bundle {
  val MR  = Bool(INPUT)        //read meta
  val MW  = Bool(INPUT)        //update the meta
  val DR  = Bool(INPUT)        //read tag from data array
  val DW  = Bool(INPUT)        //write tag to data array //equals DWR+DWB
  val WB  = Bool(INPUT)        //write dirty line back to memory
  val F   = Bool(INPUT)        //fetch the target cache line (from memory) // equals DWB
}

class TCTAGTrackerPerformCounter(w: Int=1) extends Bundle {
  val MR  = UInt(width=w)        //read meta
  val MW  = UInt(width=w)        //update the meta
  val DR  = UInt(width=w)        //read tag from data array
  val DW  = UInt(width=w)        //write tag to data array //equals DWR+DWB
  val WB  = UInt(width=w)        //write dirty line back to memory
  val F   = UInt(width=w)        //fetch the target cache line (from memory) // equals DWB
}

class TCMEMTrackerPerform extends Bundle {
  val TTW        = Bool(INPUT)   //TTW count equals TM0W/TM1W
  val TTR        = Bool(INPUT)   //TTR count
  val TTR_miss   = Bool(INPUT)   //TTR miss count
  val TM0R       = Bool(INPUT)   //TM0R count
  val TM0R_miss  = Bool(INPUT)   //TM0R miss count
  val TM1F       = Bool(INPUT)   //TM1F count
  val TM1F_miss  = Bool(INPUT)   //TM1F miss count (miss in tagcache)
}

class TCMEMTrackerPerformCounter(w: Int=1) extends Bundle {
  val TTW        = UInt(width=w)   //TTW count equals TM0W/TM1W
  val TTR        = UInt(width=w)   //TTR count
  val TTR_miss   = UInt(width=w)   //TTR miss count
  val TM0R       = UInt(width=w)   //TM0R count
  val TM0R_miss  = UInt(width=w)   //TM0R miss count
  val TM1F       = UInt(width=w)   //TM1F count
  val TM1F_miss  = UInt(width=w)   //TM1F miss count (miss in tagcache)
}

class TAGCachePerform extends Bundle {
  val tcttp = new TCTAGTrackerPerform()//
  val tcmtp = new TCMEMTrackerPerform()
}

class PrivatePerform extends Bundle {
  val L1I = new L1ICachePerform()
  val L1D = new L1DCachePerform()
}

//class SharePerform(implicit val p: Parameters) extends PFCBundle { //WRONG
//  val L2D = Vec(L2DBanks, new L2DCachePerform)
class SharePerform(implicit val p: Parameters) extends Bundle {
  val L2D = Vec(p(PFCL2N), new L2DCachePerform)
  val TAG = new TAGCachePerform()
}

class PrivatePFC extends Module {
  val io = new Bundle {
    //val req = //req from csr
    //val resp = //resp to csr
    val update = new PrivatePerform()
  }

  val acquire = Bool()

  //import rocket.WideCounter //can't import
  //val L1I_PFC_read = WideCounter(64, io.update.L1I.read)
  //val (L1IPFC_read, full0)     = Counter(io.update.L1I.read,         2^64-1) //wrong why??!!
  val l1ipfc_read           = PerFormanceCounter(io.update.L1I.read,         2^64-1)
  val l1ipfc_readmiss       = PerFormanceCounter(io.update.L1I.read_miss,    2^64-1)
  val l1dpfc_read           = PerFormanceCounter(io.update.L1D.read,         2^64-1)
  val l1dpfc_readmiss       = PerFormanceCounter(io.update.L1D.read_miss,    2^64-1)
  val l1dpfc_write          = PerFormanceCounter(io.update.L1D.write,        2^64-1)
  val l1dpfc_writemiss      = PerFormanceCounter(io.update.L1D.write_miss,   2^64-1)

  val Reg_l1ipfc_read       = RegEnable(l1ipfc_read,       acquire)
  val Reg_l1ipfc_readmiss   = RegEnable(l1ipfc_readmiss,   acquire)
  val Reg_L1IPFC_read       = RegEnable(l1dpfc_read,       acquire)
  val Reg_L1DPFC_readmiss   = RegEnable(l1dpfc_readmiss,   acquire)
  val Reg_L1DPFC_write      = RegEnable(l1dpfc_write,      acquire)
  val Reg_L1DPFC_writemiss  = RegEnable(l1dpfc_writemiss,  acquire)
}

class SharePFC(implicit val p: Parameters) extends Module {
  val io = new Bundle {
    //val req = //req from csr
    //val resp = //resp to csr
    val update = new SharePerform()
  }

  val L2DBanks = p(PFCL2N)
  val acquire = Bool()

  //L2D
  val l2dpfcs     = (0 until L2DBanks).map(i => Wire(new L2DCachePerformCounter(w=64)))
  val Reg_l2dpfcs = (0 until L2DBanks).map(i => Reg(new  L2DCachePerformCounter(w=64)))
    (0 until L2DBanks).map(i =>{
    Reg_l2dpfcs(i)         := RegEnable(l2dpfcs(i), acquire)
    l2dpfcs(i).read        := PerFormanceCounter(io.update.L2D(i).read.toBool(),        2^64-1)
    l2dpfcs(i).read_miss   := PerFormanceCounter(io.update.L2D(i).read_miss.toBool(),   2^64-1)
    l2dpfcs(i).write       := PerFormanceCounter(io.update.L2D(i).write.toBool(),       2^64-1)
    l2dpfcs(i).write_back  := PerFormanceCounter(io.update.L2D(i).write_back.toBool(),  2^64-1)
  })

  //TAG
  val tcttpfc = Wire(new TCTAGTrackerPerformCounter(w=64))
  val tcmtpfc = Wire(new TCMEMTrackerPerformCounter(w=64))
  val Reg_tcttpfc = RegEnable(tcttpfc, acquire)
  val Reg_tcmtpfc = RegEnable(tcmtpfc, acquire)
  tcttpfc.MR          := PerFormanceCounter(io.update.TAG.tcttp.MR.toBool(),          2^64-1)
  tcttpfc.DR          := PerFormanceCounter(io.update.TAG.tcttp.DR.toBool(),          2^64-1)
  tcttpfc.DW          := PerFormanceCounter(io.update.TAG.tcttp.DW.toBool(),          2^64-1)
  tcttpfc.WB          := PerFormanceCounter(io.update.TAG.tcttp.WB.toBool(),          2^64-1)
  tcttpfc.F           := PerFormanceCounter(io.update.TAG.tcttp.F.toBool(),           2^64-1)
  tcmtpfc.TTW         := PerFormanceCounter(io.update.TAG.tcmtp.TTW.toBool(),         2^64-1)
  tcmtpfc.TTR         := PerFormanceCounter(io.update.TAG.tcmtp.TTR.toBool(),         2^64-1)
  tcmtpfc.TTR_miss    := PerFormanceCounter(io.update.TAG.tcmtp.TTR_miss.toBool(),    2^64-1)
  tcmtpfc.TM0R        := PerFormanceCounter(io.update.TAG.tcmtp.TM0R.toBool(),        2^64-1)
  tcmtpfc.TM0R_miss   := PerFormanceCounter(io.update.TAG.tcmtp.TM0R_miss.toBool(),   2^64-1)
  tcmtpfc.TM1F        := PerFormanceCounter(io.update.TAG.tcmtp.TM1F.toBool(),        2^64-1)
  tcmtpfc.TM1F_miss   := PerFormanceCounter(io.update.TAG.tcmtp.TM1F_miss.toBool(),   2^64-1)
}
