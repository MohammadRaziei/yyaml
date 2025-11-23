import clsx from 'clsx';
import Link from '@docusaurus/Link';
import useDocusaurusContext from '@docusaurus/useDocusaurusContext';
import Layout from '@theme/Layout';
import HomepageFeatures from '@site/src/components/HomepageFeatures';

import Heading from '@theme/Heading';
import styles from './index.module.css';

function HomepageHeader() {
  const {siteConfig} = useDocusaurusContext();
  return (
    <header className={clsx('hero hero--primary', styles.heroBanner)}>
      <div className="container">
        <div className={styles.heroContent}>
          <div className={styles.heroText}>
            <Heading as="h1" className="hero__title">
              {siteConfig.title}
            </Heading>
            <p className="hero__subtitle">{siteConfig.tagline}</p>
            <p className={styles.heroDescription}>
              A blazing-fast YAML parser written in C with comprehensive C++ and Python bindings. 
              Perfect for performance-critical applications and embedded systems.
            </p>
            <div className={styles.buttons}>
              <Link
                className="button button--primary button--lg"
                to="/languages">
                Get Started
              </Link>
              <Link
                className="button button--outline button--lg"
                href="https://github.com/mohammadraziei/yyaml">
                View on GitHub
              </Link>
            </div>
          </div>
          <div className={styles.heroVisual}>
            <div className={styles.codeWindow}>
              <div className={styles.codeHeader}>
                <div className={styles.codeDots}>
                  <span className={styles.codeDot} style={{backgroundColor: '#ff5f56'}}></span>
                  <span className={styles.codeDot} style={{backgroundColor: '#ffbd2e'}}></span>
                  <span className={styles.codeDot} style={{backgroundColor: '#27ca3f'}}></span>
                </div>
                <span className={styles.codeTitle}>config.yaml</span>
              </div>
              <pre className={styles.codeBlock}>
{`server:
  host: localhost
  port: 8080
  features:
    - authentication
    - rate_limiting

database:
  connection_string: postgresql://...
  pool_size: 10`}</pre>
            </div>
          </div>
        </div>
      </div>
    </header>
  );
}

export default function Home() {
  const {siteConfig} = useDocusaurusContext();
  return (
    <Layout
      title={`${siteConfig.title} - High-performance YAML parsing`}
      description="yyaml - High-performance YAML parsing in C with C++ and Python bindings. Fast, memory-efficient, and cross-platform.">
      <HomepageHeader />
      <main>
        <HomepageFeatures />
      </main>
    </Layout>
  );
}
