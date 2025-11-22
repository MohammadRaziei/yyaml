import clsx from 'clsx';
import Heading from '@theme/Heading';
import styles from './styles.module.css';

const FeatureList = [
  {
    title: 'High Performance',
    description: (
      <>
        yyaml provides high-performance YAML parsing in C with optimized C++ bindings
        for maximum speed and efficiency.
      </>
    ),
  },
  {
    title: 'Multi-Language Support',
    description: (
      <>
        Available in C, C++, and Python with consistent APIs across all languages
        for seamless integration.
      </>
    ),
  },
  {
    title: 'Memory Efficient',
    description: (
      <>
        Designed with memory efficiency in mind, making it suitable for both
        embedded systems and large-scale applications.
      </>
    ),
  },
];

function Feature({title, description}) {
  return (
    <div className={clsx('col col--4')}>
      <div className="text--center padding-horiz--md">
        <Heading as="h3">{title}</Heading>
        <p>{description}</p>
      </div>
    </div>
  );
}

export default function HomepageFeatures() {
  return (
    <section className={styles.features}>
      <div className="container">
        <div className="row">
          {FeatureList.map((props, idx) => (
            <Feature key={idx} {...props} />
          ))}
        </div>
      </div>
    </section>
  );
}
