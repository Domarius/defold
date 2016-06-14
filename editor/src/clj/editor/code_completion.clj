(ns editor.code-completion
  (:require [clojure.string :as string]
            [clojure.set :as set]
            [editor.defold-project :as project]
            [editor.lua :as lua]
            [editor.lua-parser :as lua-parser]
            [editor.resource :as resource]
            [dynamo.graph :as g]))

(defn make-completions [resource-completion include-locals?]
  (let [{:keys [vars local-vars functions local-functions]} resource-completion
        var-info (if include-locals? (set/union vars local-vars) vars)
        vars (map (fn [v] {:name v
                          :display-string v
                          :additional-info ""})
                  var-info)
        fn-info (if include-locals? (merge functions local-functions) functions)
        fns  (map (fn [[fname {:keys [params]}]]
                    {:name fname
                     :display-string (str fname "("
                                          (apply str (interpose "," (map #(str "[\"" % "\"]") params)))
                                          ")")
                     :additional-info ""})
                  fn-info)]
    (vec (concat vars fns))))

(defn find-module-node-in-project [node-id module-name]
  (let [project-node (project/get-project node-id)
        resource-node-pairs (g/node-value project-node :nodes-by-resource-path)
        results (filter #(= (lua/lua-module->path module-name) (first %)) resource-node-pairs)]
    (when (= 1 (count results))
      (let [[_ result-node-id] (first results)]
        (when  (= "editor.script/ScriptNode" (-> result-node-id (g/node-by-id) (g/node-type) :name))
          result-node-id)))))

(defn resource-node-path [nid]
  (resource/proj-path (g/node-value nid :resource)))

(defn find-module-node [node-id module-name module-nodes]
  (when module-name
   (let [rpath (lua/lua-module->path module-name)
         module-node-id (some (fn [nid] (if (= rpath (resource-node-path nid)) nid)) module-nodes)]
     (if module-node-id
       module-node-id
       (when-let [nid (find-module-node-in-project node-id module-name)]
         (g/transact (g/connect nid :_node-id node-id :module-nodes))
         nid)))))

(defn gather-requires [node-id ralias rname module-nodes]
  (let [rnode (find-module-node node-id rname module-nodes)
        rcompletion-info (g/node-value rnode :completion-info)]
    {ralias (make-completions rcompletion-info false)}))

(defn combine-completions [_node-id system-docs completion-info module-nodes]
 (let [base (merge system-docs {"" (make-completions completion-info true)})
       require-info (or (:requires completion-info) {})
       require-completions (apply merge
                                  (map (fn [[ralias rname]] (gather-requires _node-id ralias rname module-nodes)) require-info))]
   (merge base require-completions)))
