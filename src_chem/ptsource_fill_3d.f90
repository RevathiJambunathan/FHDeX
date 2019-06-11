subroutine get_ptsource_3d( lo, hi, &
     &            iface, if_lo, if_hi, &
     &            ptS, pts_lo, pts_hi, &
     &            strength, dx,ib_cen_x,&
     &            ib_cen_y, ib_cen_z, prob_lo) bind(C, name="get_ptsource_3d")
  
  use amrex_mempool_module, only : bl_allocate, bl_deallocate

  implicit none
  double precision, intent(in) :: strength, prob_lo(3), dx(3)
  double precision, intent(in) :: ib_cen_x, ib_cen_y, ib_cen_z
  integer, intent(in) :: lo(3), hi(3)
  integer, intent(in) :: if_lo(3), if_hi(3)
  integer, intent(in) :: pts_lo(3), pts_hi(3)
  integer, intent(in) :: iface(if_lo(1):if_hi(1),if_lo(2):if_hi(2),if_lo(3):if_hi(3))
  double precision, intent(out) :: ptS(pts_lo(1):pts_hi(1),pts_lo(2):pts_hi(2),pts_lo(3):pts_hi(3))

  integer :: i, j, k
  double precision:: z

 do       k = lo(3), hi(3)
    do    j = lo(2), hi(2)
       do i = lo(1), hi(1)
          z = prob_lo(3) + (dble(k)+0.5d0) * dx(3)
           if ((iface(i,j,k).eq.1) .and. (z .le. ib_cen_z)) then

           pts(i,j,k)=strength
           endif
        enddo
     enddo
  enddo

end subroutine get_ptsource_3d