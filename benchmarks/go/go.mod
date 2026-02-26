module github.com/mohammadraziei/yyaml/benchmarks/go

go 1.24.0

replace github.com/mohammadraziei/yyaml => ../..

require (
	github.com/Shopify/yaml v2.1.0+incompatible
	github.com/ghodss/yaml v1.0.0
	github.com/go-yaml/yaml v2.1.0+incompatible
	github.com/goccy/go-yaml v1.19.2
	github.com/kylelemons/go-gypsy v1.0.0
	github.com/mohammadraziei/yyaml v0.0.0
	gopkg.in/yaml.v3 v3.0.1
	sigs.k8s.io/yaml v1.4.0
)

require gopkg.in/yaml.v2 v2.4.0 // indirect
