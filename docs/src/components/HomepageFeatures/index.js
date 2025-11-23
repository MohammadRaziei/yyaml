import clsx from 'clsx';
import Heading from '@theme/Heading';
import styles from './styles.module.css';

const FeatureList = [
  {
    title: '🚀 High Performance',
    description: (
      <>
        yyaml provides high-performance YAML parsing in C with optimized C++ bindings
        for maximum speed and efficiency. Built for performance-critical applications.
      </>
    ),
  },
  {
    title: '🌐 Multi-Language Support',
    description: (
      <>
        Available in C, C++, and Python with consistent APIs across all languages
        for seamless integration. Write once, use everywhere.
      </>
    ),
  },
  {
    title: '💾 Memory Efficient',
    description: (
      <>
        Designed with memory efficiency in mind, making it suitable for both
        embedded systems and large-scale applications. Minimal footprint, maximum power.
      </>
    ),
  },
];

function Feature({title, description}) {
  return (
    <div className={clsx('col col--4')}>
      <div className={styles.feature}>
        <div className={styles.featureIcon}>
          <div className={styles.iconBackground}></div>
        </div>
        <Heading as="h3" className={styles.featureTitle}>{title}</Heading>
        <p className={styles.featureDescription}>{description}</p>
      </div>
    </div>
  );
}

export default function HomepageFeatures() {
  return (
    <section className={styles.features}>
      <div className="container">
        <div className={styles.featuresHeader}>
          <Heading as="h2" className={styles.featuresTitle}>Why Choose yyaml?</Heading>
          <p className={styles.featuresSubtitle}>
            Built for developers who demand performance and reliability
          </p>
        </div>
        <div className="row">
          {FeatureList.map((props, idx) => (
            <Feature key={idx} {...props} />
          ))}
        </div>
      </div>
    </section>
  );
}
