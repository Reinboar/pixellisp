(defun (reduce f l a)
    (if (null? l)
        a
        (reduce f (cdr l) (f a (car l)))))

(define foldl reduce)

(define L lambda)

(defun (foldr f l a)
    (if (null? l)
        a
        (f (foldr f (cdr l) a) (car l))))

(defun (map f l)
    (if (null? l)
        l
        (cons (f (car l))
              (map f (cdr l)))))

(defun (for-each f l)
    (if (null? l)
        null
        (begin (f (car l))
               (for-each f (cdr l)))))

(defun (partition f l)
    (cons (filter f l)
          (remove f l)))

(defun (filter f l)
    (if (null? l)
        l
        (if (f (car l))
            (cons (car l) (filter f (cdr l)))
            (filter f (cdr l)))))

(defun (remove f l)
    (filter (L (x) (not (f x))) l))

(defun (sum l)
    (reduce + l 0))

(defun (iota n)
    (if (< n 1)
        null
        (let inner-iota ([n (- n 1)]
                        [result null])
                (if (= n 0)
                    (cons n result)
                    (inner-iota (- n 1) (cons n result))))))

(defun (even? n)
    (= (% n 2) 0))

(defun (odd? n)
    (not (even? n)))

(defun (proc? x)
    (or (lambda? x) (primitive? x)))

(defun (list x . xs)
    (let inner-list ([src    (reverse (cons x xs))]
                     [result null])
            (if (null? (cdr src))
                (cons (car src) result)
                (inner-list (cdr src) (cons (car src) result)))))

(defun (cons* x . xs)
    (if (null? xs)
        x
        (cons x (apply cons* xs))))

(defun (make-list n . fill)
        (let inner-make ([n      n]
                         [fill   (if (null? fill) 0 (car fill))]
                         [result null])
                (if (= n 0)
                    result
                    (inner-make (- n 1)
                                fill
                                (cons fill result)))))

(defun (cadr l)
    (car (cdr l)))

(defun (reverse l)
    (foldl (lambda (a e)
               (cons e a))
        (cdr l)
        (cons (car l) null)))

(defmacro (let* args . body)
    (foldr (lambda (a e)
                `(let ([,(car e) ,(cadr e)])
                    ,a))
            args `(begin ,@body)))

(defun (zip-pair ks vs)
    (if (or (null? ks)
            (null? vs))
        null
        (cons (cons (car ks) (car vs))
              (zip (cdr ks) (cdr vs)))))

(defun (zip ks vs)
    (if (or (null? ks)
            (null? vs))
        null
        (cons (list (car ks) (car vs))
              (zip (cdr ks) (cdr vs)))))

(defmacro (when cond . body)
    `(if ,cond
         (begin ,(car body))
         null))

(defun (length l)
    (reduce (lambda (a e)
                (+ a 1))
            l 0))

(defmacro (apply f args)
    `(,f ,@(eval args)))

(defun (andmap f l)
    (reduce (lambda (a e)
                (and (f e) a))
            l true))

(defun (ormap f l)
    (reduce (lambda (a e)
                (or (f e) a))
            l false))

(defun (last-pair p)
    (if (null? (cdr p))
        p
        (last-pair (cdr p))))

(defun (last p)
    (car (last-pair p)))

(defun (identity x) x)

(defun (drop l i)
    (if (= i 0)
        l
        (drop (cdr l) (- i 1))))

(defun (take l i)
    (let inner-take ([l l]
                     [i i]
                     [result null])
        (if (= i 0)
            (reverse result)
            (inner-take (cdr l) (- i 1) (cons (car l) result)))))

(defun (circular-list x . xs)
    (define root (list x))
    (set-cdr! (last-pair xs) root)
    (set-cdr! root xs)
    root)

(defmacro (destructure names l)
    `(begin ,@(map (L (x)
                     `(define ,@x))
                   (zip names (eval l)))))

(defun (list-tail l n)
    (if (= n 0)
        l
        (list-tail (cdr l) (- n 1))))

(defun (list-ref l n)
    (if (= n 0)
        (car l)
        (list-ref (cdr l) (- n 1))))

(defmacro (unless cond body)
    `(if (not ,cond)
         ,body
         null))

(defmacro (cond exp . exps)
    (foldr (L (a e)
                `(if ,(car e) (begin ,@(cdr e))
                    ,a))
            (cons exp exps) 'null))

(defun (fib n)
    (let fib-inner ([a 0]
                    [b 1]
                    [n n])
                (if (= n 0)
                    a
                    (fib-inner b (+ a b) (- n 1)))))

(defun (make-promise proc)
    (let ([result-ready? false]
          [result false])
        (L (_)
            (if result-ready?
                result
                (let ([x (proc)])
                    (if result-ready?
                        result
                        (begin (set! result-ready? true)
                               (set! result x)
                               result)))))))

(defmacro (delay exp)
    `(make-promise (L (_) ,exp)))

(defun (compose f g)
    (L (x)
        (g (f x))))

(define first  car)
(define second (compose cdr first))
(define third  (compose cdr second))
(define fourth (compose cdr third))
(define fifth  (compose cdr fourth))

(defun (copy-list l)
    (map (L (x) x) l))

(defun (append l1 l2)
    (define new-l1 (copy-list l1))
    (define new-l2 (copy-list l2))
    (cond [(and (null? new-l1) (null? new-l2))
                null]
          [(and (null? new-l1) (not (null? new-l2)))
                new-l2]
          [(and (not (null? new-l1)) (null? new-l2))
                new-l1]
          [true `(,@new-l1 ,@new-l2)]))

(defun (concatenate ls)
    (reduce append ls null))

(defmacro (loop x . xs)
    `(let inner-loop ([z 0])
        (begin ,@(cons x xs)
                 (inner-loop z))))

(defun (abs n)
    (if (< n 0)
        (* n (- 0 1))
        n))

(defun (make-lcg seed)
    (let* ([m 2147483647]
           [a 1103515245]
           [c 14730763]
           [state seed])
        (L (min max)
            (set! state (% (+ (* a state) c) m))
            (+ (% (abs state)
                  (- (+ max 1) min))
               min))))

(defmacro (make-dispatcher name . cmd-pairs)
    (list 'begin
            (cons 'begin (map (L (p)
                    (list 'defun (cons (car p) (cons name 'val)) (list name (list 'quote (car p)) 'val)))
                    cmd-pairs))
            (list 'L (list 'init)
                (list 'let (list (list name 'init))
                    (list 'L (cons 'cmd 'val)
                        (list 'set! 'val (list 'car 'val))
                        (cons 'cond (map (L (p)
                                        (cons (list 'eqv? 'cmd (list 'quote (car p))) (cdr p)))
                                        cmd-pairs)))))))

(defun (fact n)
    (let inner-fact ([n n]
                     [result 1])
        (if (< n 2)
            result
            (inner-fact (- n 1) (* n result)))))

(defun (make-vector len . val)
    (if (null? val)
        (apply vector (make-list len))
        (apply vector (make-list len (car val)))))

(defun (vector-for-each f vec)
    (let vec-loop ([i 0])
            (if (= i (vector-length vec))
                null
                (begin
                    (f (vector-ref vec i))
                    (vec-loop (+ i 1))))))

(defun (vector-copy vec)
    (let ([new-vec (make-vector (vector-length vec))])
        (let loop ([i 0])
            (if (= i (vector-length vec))
                new-vec
                (begin
                    (vector-set! new-vec i (vector-ref vec i))
                    (loop (+ i 1)))))))

(defun (vector-map! f vec)
    (let vec-loop ([i 0])
        (if (= i (vector-length vec))
            vec
            (begin
                (vector-set! vec i (f (vector-ref vec i)))
                (vec-loop (+ i 1))))))

(defun (vector-map f vec)
    (vector-map! f (vector-copy vec)))

(defun (vector-reduce f vec init)
    (let loop ([i 0]
               [result init])
            (if (= i (vector-length vec))
                result
                (begin
                    (print "I=" i "RESULT=" result "LENGTH=" (vector-length vec) "ELEMENT=" (vector-ref vec i))
                    (loop (+ i 1)
                          (f result (vector-ref vec i)))))))

(defun (vector->list vec)
    (let loop ([i (- (vector-length vec) 1)]
               [result null])
            (if (= i 0)
                (cons (vector-ref vec i) result)
                (loop (- i 1)
                      (cons (vector-ref vec i) result)))))

(defmacro (begin0 fst . rst)
    `(let ([result (eval ,fst)])
        (begin ,@rst result)))

(defun (gen-accessor-defuns struct-name field-names)
    (let ([struct-name-str (symbol->string struct-name)])
        (let loop ([field-names field-names]
                   [i 1])
                (if (null? field-names)
                    null
                    (let* ([fname-str (symbol->string (car field-names))]
                           [getter-name (string->symbol (conc struct-name-str "-" fname-str))]
                           [setter-name (string->symbol (conc struct-name-str "-" fname-str "-set!"))])
                        (cons `(defun (,getter-name obj)
                                  (vector-ref obj ,i))
                               (cons `(defun (,setter-name obj val)
                                            (vector-set! obj ,i val))
                                     (loop (cdr field-names)
                                           (+ i 1)))))))))

(defmacro (define-structure struct-name . field-names)
    (let ([field-num   (length field-names)]
          [constr-name (string->symbol (conc "make-" (symbol->string struct-name)))]
          [pred-name   (string->symbol (conc (symbol->string struct-name) "?"))]
          [field-procs (gen-accessor-defuns struct-name field-names)])
        `(begin (defun (,constr-name ,@field-names)
                    (vector ',struct-name ,@field-names))
                (defun (,pred-name obj)
                    (and (vector? obj)
                         (eqv? (vector-ref obj 0) ',struct-name)))
                ,@field-procs)))
