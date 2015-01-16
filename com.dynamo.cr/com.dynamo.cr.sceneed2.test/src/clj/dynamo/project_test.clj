(ns dynamo.project-test
  (:require [clojure.test :refer :all]
            [dynamo.node :as n]
            [dynamo.project :as p]
            [dynamo.file :as f]
            [dynamo.system :as ds]
            [dynamo.system.test-support :refer [with-clean-world]]
            [dynamo.types :as t]
            [clojure.java.io :as io])
  (:import [java.io StringReader]))

(n/defnode DummyNode)

(defrecord ExtensionHolder [ext]
  t/PathManipulation
  (extension [this] ext)

  io/IOFactory
  (make-reader [this opts] (StringReader. "")))

(deftest make-editor
  (testing "throws if return value is not a node"
    (with-clean-world
      (let [project-node (ds/transactional
                           (ds/in (ds/add (n/construct p/Project))
                             (p/register-loader "dummy" (fn [& _] (n/construct DummyNode)))
                             (p/register-editor "dummy" (constantly :not-a-node))
                             (ds/current-scope)))]
        (is (thrown-with-msg? AssertionError #"must return a node"
              (p/make-editor project-node (ExtensionHolder. "dummy") nil)))))))

(deftest find-nodes-by-extension
  (with-clean-world
    (let [d1           (n/construct DummyNode :filename (f/native-path "foo.png"))
          d2           (n/construct DummyNode :filename (f/native-path "/var/tmp/foo.png"))
          d3           (n/construct DummyNode :filename (f/native-path "foo.script"))
          d4           (n/construct DummyNode :is-a-file-node? false)
          [project-node d1 d2 d3 d4] (ds/transactional
                                         (ds/in (ds/add (n/construct p/Project))
                                           [(ds/current-scope) (ds/add d1) (ds/add d2) (ds/add d3) (ds/add d4)]))]
      (is (= #{d1 d2}    (p/nodes-with-extensions project-node ["png"])))
      (is (= #{d3}       (p/nodes-with-extensions project-node ["script"])))
      (is (= #{d1 d2 d3} (p/nodes-with-extensions project-node ["png" "script"]))))))
