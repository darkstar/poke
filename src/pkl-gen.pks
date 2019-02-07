;;; -*- mode: asm -*-
;;; pkl-gen.pks - Assembly routines for the codegen
;;;

;;; Copyright (C) 2019 Jose E. Marchesi

;;; This program is free software: you can redistribute it and/or modify
;;; it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation, either version 3 of the License, or
;;; (at your option) any later version.
;;;
;;; This program is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY ; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License
;;; along with this program.  If not, see <http: //www.gnu.org/licenses/>.

;;; Note that map-related routines live in their dedicated file:
;;; pkl-gen-maps.pks.

        .macro offset_cast

;;; RAS_MACRO_OFFSET_CAST
;;; ( OFF -- OFF )
;;;
;;; This macro generates code that converts an offset of a given type
;;; into an offset of another given type.
;;;
;;; The following C variables shall be available to the macro
;;; expansion:
;;;
;;; `from_base_type'
;;;    pkl_ast_node reflecting the type of the source offset magnitude.
;;; `from_base_unit_type'
;;;    pkl_ast_node reflecting the type of the source offset unit.
;;; 
;;; `to_base_type'
;;;    pkl_ast_node reflecting the type of the result offset magnitude.
;;; `to_base_unit_type'
;;;    pkl_ast_node reflecting the type of the result offset unit.

        ;; XXX use OGETMC here.

        ;; XXX we have to do the arithmetic in base_unit_types, then
        ;; convert to to_base_type, to assure that to_base_type can hold
        ;; the to_base_unit.  Otherwise weird division by zero occurs.
        
        ;; Get the magnitude of the offset and convert it to the new
        ;; magnitude type.
        ogetm                   ; OFF OFFM

        .c if (!pkl_ast_type_equal (from_base_type, to_base_type))
        .c {
              nton from_base_type, to_base_type ; OFF OFFM OFFMC
              nip                               ; OFF OFFMC
        .c }

        ;; Now do the same for the unit.
        ;; XXX can this subpass be replaced with ogetu?
        .c PKL_PASS_SUBPASS (from_base_unit);
                                ; OFF OFFMC OFFU
        .c if (!pkl_ast_type_equal (from_base_unit_type, to_base_type))
        .c {
             nton from_base_unit_type, to_base_type ; OFF OFFMC OFFU OFFUC
             nip                                    ; OFF OFFMC OFFUC
        .c }

        mul to_base_type        ; OFF OFFMC OFFUC (OFFMC*OFFUC)
        nip2                    ; OFF (OFFMC*OFFUC)


        ;; Convert the new unit.
        .c PKL_PASS_SUBPASS (to_base_unit);
                                ; OFF (OFFMC*OFFUC) NUNIT
        .c if (!pkl_ast_type_equal (to_base_unit_type, to_base_type))
        .c {
             nton to_base_unit_type, to_base_type ; OFF (OFFMC*OFFUNC) NUNIT NUNITC
             nip                                  ; OFF (OFFMC*OFFUNC) NUNITC
        .c }

        div to_base_type
        nip2                    ; OFF (OFFMC*OFFUNC/NUNITC)
        swap                    ; (OFFMC*OFFUNC/NUNITC) OFF
        .c PKL_PASS_SUBPASS (to_base_unit);
                                ; (OFFMC*OFFUNC/NUNITC) OFF NUNIT
        nip                     ; (OFFMC*OFFUNC/NUNITC) NUNIT
        mko                     ; OFFC
        
        .end                    ; offset_cast

