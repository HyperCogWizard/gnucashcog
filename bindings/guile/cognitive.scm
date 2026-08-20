;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; cognitive.scm -- Guile module (gnucash cognitive)
;; Copyright (C) 2024-2026 GnuCash Cognitive Engine
;;
;; Friendly Scheme surface over the SWIG-exported cognitive C API.
;; Never evaluates untrusted book data as Scheme.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-module (gnucash cognitive))

(eval-when (compile load eval expand)
  (load-extension "libgnucash-guile" "gnc_guile_bindings_init"))

(use-modules (sw_engine))
(use-modules (srfi srfi-1))

(export gnc-cognitive-init
        gnc-cognitive-shutdown
        gnc-cognitive-initialized?
        gnc-cognitive-set-auto!
        gnc-cognitive-auto?
        cognitive-backend-name
        cognitive-backend-select!
        cognitive-backend-available?
        cognitive-backend-status-json
        cognitive-backend-health-check?
        cognitive-observe-book!
        cognitive-on-commit!
        cognitive-pln-validate
        cognitive-pln-trial-balance-balanced?
        cognitive-pln-last-score
        cognitive-ecan-sti
        cognitive-ecan-lti
        cognitive-ecan-decay!
        cognitive-tx-badge
        cognitive-tx-badge-label
        cognitive-account-heat
        cognitive-account-heat-color
        cognitive-ui-badges-enabled?
        cognitive-account->scheme
        cognitive-transaction->scheme-pattern
        cognitive-hypergraph-export
        cognitive-moses-strategies-json
        cognitive-html-summary
        cognitive-attention-html
        cognitive-validation-html)

;; ---------------------------------------------------------------------------
;; Lifecycle
;; ---------------------------------------------------------------------------

(define (gnc-cognitive-init)
  (gnc-cognitive-accounting-init))

(define (gnc-cognitive-shutdown)
  (gnc-cognitive-accounting-shutdown))

(define (gnc-cognitive-initialized?)
  (gnc-cognitive-accounting-is-initialized))

(define (gnc-cognitive-set-auto! enabled?)
  (gnc-cognitive-accounting-set-auto-enabled enabled?))

(define (gnc-cognitive-auto?)
  (gnc-cognitive-accounting-get-auto-enabled))

;; ---------------------------------------------------------------------------
;; Backend
;; ---------------------------------------------------------------------------

(define (cognitive-backend-name)
  (gnc-cognitive-backend-name))

(define (cognitive-backend-select! kind-symbol)
  "Select backend: 'simulated or 'opencog."
  (let ((kind (case kind-symbol
                ((simulated sim) 0)
                ((opencog) 1)
                (else 0))))
    (gnc-cognitive-backend-select kind)))

(define (cognitive-backend-available? kind-symbol)
  (let ((kind (case kind-symbol
                ((simulated sim) 0)
                ((opencog) 1)
                (else -1))))
    (and (>= kind 0)
         (gnc-cognitive-backend-available kind))))

(define (cognitive-backend-status-json)
  (gnc-cognitive-backend-status-json))

(define (cognitive-backend-health-check?)
  (gnc-cognitive-backend-health-check))

(define (cognitive-observe-book! book)
  (gnc-cognitive-backend-sync-book book))

(define (cognitive-on-commit! txn)
  (gnc-cognitive-backend-sync-transaction txn))

;; ---------------------------------------------------------------------------
;; PLN / ECAN / UI
;; ---------------------------------------------------------------------------

(define (cognitive-pln-validate txn)
  (gnc-pln-validate-double-entry txn))

(define (cognitive-pln-trial-balance-balanced? root-account)
  (gnc-pln-trial-balance-balanced root-account))

(define (cognitive-pln-last-score txn)
  (gnc-pln-get-last-validation-score txn))

(define (cognitive-ecan-sti account)
  (gnc-ecan-account-sti account))

(define (cognitive-ecan-lti account)
  (gnc-ecan-account-lti account))

(define (cognitive-ecan-decay!)
  (gnc-ecan-decay-tick))

(define (cognitive-tx-badge txn)
  "Return badge symbol: ok | warn | fail | unknown."
  (case (gnc-cognitive-transaction-badge txn)
    ((0) 'ok)
    ((1) 'warn)
    ((2) 'fail)
    (else 'unknown)))

(define (cognitive-tx-badge-label txn)
  (gnc-cognitive-transaction-badge-label txn))

(define (cognitive-account-heat account)
  (gnc-cognitive-account-attention-heat account))

(define (cognitive-account-heat-color account)
  (gnc-cognitive-account-attention-css-color account))

(define (cognitive-ui-badges-enabled?)
  (gnc-cognitive-ui-badges-enabled))

;; ---------------------------------------------------------------------------
;; Safe export strings / HTML
;; ---------------------------------------------------------------------------

(define (cognitive-account->scheme account)
  (gnc-account-to-scheme-representation account))

(define (cognitive-transaction->scheme-pattern txn)
  (gnc-transaction-to-scheme-pattern txn))

(define (cognitive-hypergraph-export root-account)
  (gnc-create-hypergraph-pattern-encoding root-account))

(define (cognitive-moses-strategies-json)
  (gnc-moses-last-strategies-json))

(define (cognitive-html-summary book)
  (gnc-cognitive-html-summary-for-book book))

(define (cognitive-attention-html book top-n)
  (gnc-cognitive-attention-table-html book top-n))

(define (cognitive-validation-html book)
  (gnc-cognitive-validation-summary-html book))
