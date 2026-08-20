;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; cognitive-accounting.scm -- Cognitive accounting HTML report
;; Copyright (C) 2024-2026 GnuCash Cognitive Engine
;;
;; Surfaces PLN proofs, ECAN attention heat, backend status, and
;; MOSES strategy JSON via the (gnucash cognitive) module.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-module (gnucash reports standard cognitive-accounting))

(use-modules (gnucash engine))
(use-modules (gnucash utilities))
(use-modules (gnucash core-utils))
(use-modules (gnucash app-utils))
(use-modules (gnucash report))
(use-modules (gnucash cognitive))

(define reportname (N_ "Cognitive Accounting"))

(define optname-report-title (N_ "Report Title"))
(define opthelp-report-title (N_ "Title for this report."))
(define optname-top-n (N_ "Top attention accounts"))
(define opthelp-top-n (N_ "How many ECAN-ranked accounts to list."))
(define optname-show-export (N_ "Include hypergraph export"))
(define opthelp-show-export
  (N_ "Append a Scheme hypergraph export of the root account (read-only)."))

(define (options-generator)
  (let* ((options (gnc:new-options))
         (optiondb (options #t)))
    (gnc-register-string-option optiondb
      gnc:pagename-general optname-report-title
      "a" opthelp-report-title (G_ "Cognitive Accounting"))
    (gnc-register-number-range-option optiondb
      gnc:pagename-general optname-top-n
      "b" opthelp-top-n
      10.0 1.0 50.0 1.0)
    (gnc-register-simple-boolean-option optiondb
      gnc:pagename-general optname-show-export
      "c" opthelp-show-export #f)
    options))

(define (cognitive-renderer report-obj)
  (define (get-option section name)
    (gnc-optiondb-lookup-value (gnc:optiondb (gnc:report-options report-obj))
                               section name))

  (let* ((document (gnc:make-html-document))
         (title (get-option gnc:pagename-general optname-report-title))
         (top-n (inexact->exact (round (get-option gnc:pagename-general optname-top-n))))
         (show-export? (get-option gnc:pagename-general optname-show-export))
         (book (gnc-get-current-book))
         (root (gnc-book-get-root-account book)))

    (gnc:html-document-set-title! document title)

    ;; Ensure cognitive core is live for the report session.
    (unless (gnc-cognitive-initialized?)
      (gnc-cognitive-init))
    (cognitive-observe-book! book)

    (gnc:html-document-add-object!
     document
     (gnc:make-html-text
      (gnc:html-markup-h2 (G_ "Backend & AtomSpace"))
      (cognitive-html-summary book)))

    (gnc:html-document-add-object!
     document
     (gnc:make-html-text
      (gnc:html-markup-h2 (G_ "ECAN Attention"))
      (cognitive-attention-html book top-n)))

    (gnc:html-document-add-object!
     document
     (gnc:make-html-text
      (gnc:html-markup-h2 (G_ "PLN Validation"))
      (cognitive-validation-html book)))

    (when (and show-export? root)
      (gnc:html-document-add-object!
       document
       (gnc:make-html-text
        (gnc:html-markup-h2 (G_ "Hypergraph export (read-only)"))
        (gnc:html-markup "pre"
         (cognitive-hypergraph-export root)))))

    (gnc:html-document-add-object!
     document
     (gnc:make-html-text
      (gnc:html-markup-p
       (G_ "Cognitive badges and attention heat are advisory. \
They never silently rewrite transactions."))))

    document))

(gnc:define-report
 'version 1
 'name reportname
 'report-guid "c0a11e7c0a11e7c0a11e7c0a11e7c0a1"
 'menu-name (N_ "Cognitive Accounting")
 'menu-tip (N_ "PLN proofs, ECAN attention, and cognitive backend status.")
 'menu-path (list gnc:menuname-experimental)
 'options-generator options-generator
 'renderer cognitive-renderer)
